
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "mapper_internal.h"
#include "types_internal.h"
#include <mapper/mapper.h>

/*! Reallocate memory used by connection. */
static void reallocate_connection_histories(mapper_connection c,
                                            int input_history_size,
                                            int output_history_size);

static void mhist_realloc(mapper_signal_history_t *history,
                          int history_size,
                          int sample_size,
                          int is_output);

const char* mapper_boundary_action_strings[] =
{
    "none",        /* BA_NONE */
    "mute",        /* BA_MUTE */
    "clamp",       /* BA_CLAMP */
    "fold",        /* BA_FOLD */
    "wrap",        /* BA_WRAP */
};

const char* mapper_mode_type_strings[] =
{
    NULL,          /* MO_UNDEFINED */
    "bypass",      /* MO_BYPASS */
    "linear",      /* MO_LINEAR */
    "expression",  /* MO_EXPRESSION */
    "calibrate",   /* MO_CALIBRATE */
    "reverse",     /* MO_REVERSE */
};

const char *mapper_get_boundary_action_string(mapper_boundary_action bound)
{
    die_unless(bound < N_MAPPER_BOUNDARY_ACTIONS && bound >= 0,
               "called mapper_get_boundary_action_string() with "
               "bad parameter.\n");

    return mapper_boundary_action_strings[bound];
}

const char *mapper_get_mode_type_string(mapper_mode_type mode)
{
    die_unless(mode < N_MAPPER_MODE_TYPES && mode >= 0,
               "called mapper_get_mode_type_string() with "
               "bad parameter.\n");

    return mapper_mode_type_strings[mode];
}

int mapper_connection_perform(mapper_connection connection,
                              mapper_signal_history_t *from,
                              mapper_signal_history_t *to)
{
    /* Currently expressions on vectors are not supported by the
     * evaluator.  For now, we half-support it by performing
     * element-wise operations on each item in the vector. */

    int changed = 0, i;

    if (connection->props.muted)
        return 0;

    /* If the destination type is unknown, we can't do anything
     * intelligent here -- even bypass mode might screw up if we
     * assume the types work out. */
    if (connection->props.dest_type != 'f'
        && connection->props.dest_type != 'i'
        && connection->props.dest_type != 'd')
    {
        return 0;
    }

    if (!connection->props.mode || connection->props.mode == MO_BYPASS)
    {
        /* Increment index position of output data structure. */
        to->position = (to->position + 1) % to->size;
        if (connection->props.src_type == connection->props.dest_type) {
            memcpy(msig_history_value_pointer(*to),
                   msig_history_value_pointer(*from),
                   mapper_type_size(to->type) * to->length);
        }
        else if (connection->props.src_type == 'f') {
            float *vfrom = msig_history_value_pointer(*from);
            if (connection->props.dest_type == 'i') {
                int *vto = msig_history_value_pointer(*to);
                for (i = 0; i < to->length; i++) {
                    vto[i] = (int)vfrom[i];
                }
            }
            else if (connection->props.dest_type == 'd') {
                double *vto = msig_history_value_pointer(*to);
                for (i = 0; i < to->length; i++) {
                    vto[i] = (double)vfrom[i];
                }
            }
        }
        else if (connection->props.src_type == 'i') {
            int *vfrom = msig_history_value_pointer(*from);
            if (connection->props.dest_type == 'f') {
                float *vto = msig_history_value_pointer(*to);
                for (i = 0; i < to->length; i++) {
                    vto[i] = (float)vfrom[i];
                }
            }
            else if (connection->props.dest_type == 'd') {
                double *vto = msig_history_value_pointer(*to);
                for (i = 0; i < to->length; i++) {
                    vto[i] = (double)vfrom[i];
                }
            }
        }
        else if (connection->props.src_type == 'd') {
            double *vfrom = msig_history_value_pointer(*from);
            if (connection->props.dest_type == 'i') {
                int *vto = msig_history_value_pointer(*to);
                for (i = 0; i < to->length; i++) {
                    vto[i] = (int)vfrom[i];
                }
            }
            else if (connection->props.dest_type == 'f') {
                float *vto = msig_history_value_pointer(*to);
                for (i = 0; i < to->length; i++) {
                    vto[i] = (float)vfrom[i];
                }
            }
        }
        return 1;
    }
    else if (connection->props.mode == MO_EXPRESSION
             || connection->props.mode == MO_LINEAR)
    {
        die_unless(connection->expr!=0, "Missing expression.\n");
        return (mapper_expr_evaluate(connection->expr, from, to));
    }

    else if (connection->props.mode == MO_CALIBRATE)
    {
        /* Increment index position of output data structure. */
        to->position = (to->position + 1) % to->size;

        if (!connection->props.range.src_min) {
            connection->props.range.src_min =
                (mval*) malloc(connection->props.src_length * sizeof(mval));
        }
        if (!connection->props.range.src_max) {
            connection->props.range.src_max =
                (mval*) malloc(connection->props.src_length * sizeof(mval));
        }

        /* If calibration mode has just taken effect, first data
         * sample sets source min and max */
        if (!connection->calibrating) {
            if (connection->props.src_type == 'f') {
                float *v = msig_history_value_pointer(*from);
                for (i = 0; i < from->length; i++) {
                    connection->props.range.src_min[i].f = v[i];
                    connection->props.range.src_max[i].f = v[i];
                }
            }
            else if (connection->props.src_type == 'i') {
                int *v = msig_history_value_pointer(*from);
                for (i = 0; i < from->length; i++) {
                    connection->props.range.src_min[i].i32 = v[i];
                    connection->props.range.src_max[i].i32 = v[i];
                }
            }
            else if (connection->props.src_type == 'd') {
                double *v = msig_history_value_pointer(*from);
                for (i = 0; i < from->length; i++) {
                    connection->props.range.src_min[i].d = v[i];
                    connection->props.range.src_max[i].d = v[i];
                }
            }
            connection->calibrating = 1;
            changed = 1;
        }
        else {
            if (connection->props.src_type == 'f') {
                float *v = msig_history_value_pointer(*from);
                for (i = 0; i < from->length; i++) {
                    if (v[i] < connection->props.range.src_min[i].f) {
                        connection->props.range.src_min[i].f = v[i];
                        changed = 1;
                    }
                    if (v[i] > connection->props.range.src_max[i].f) {
                        connection->props.range.src_max[i].f = v[i];
                        changed = 1;
                    }
                }
            }
            else if (connection->props.src_type == 'i') {
                int *v = msig_history_value_pointer(*from);
                for (i = 0; i < from->length; i++) {
                    if (v[i] < connection->props.range.src_min[i].i32) {
                        connection->props.range.src_min[i].i32 = v[i];
                        changed = 1;
                    }
                    if (v[i] > connection->props.range.src_max[i].i32) {
                        connection->props.range.src_max[i].i32 = v[i];
                        changed = 1;
                    }
                }
            }
            else if (connection->props.src_type == 'd') {
                double *v = msig_history_value_pointer(*from);
                for (i = 0; i < from->length; i++) {
                    if (v[i] < connection->props.range.src_min[i].d) {
                        connection->props.range.src_min[i].d = v[i];
                        changed = 1;
                    }
                    if (v[i] > connection->props.range.src_max[i].d) {
                        connection->props.range.src_max[i].d = v[i];
                        changed = 1;
                    }
                }
            }
        }

        if (changed) {
            mapper_connection_set_mode_linear(connection);

            /* Stay in calibrate mode. */
            connection->props.mode = MO_CALIBRATE;
        }

        if (connection->expr)
            return (mapper_expr_evaluate(connection->expr, from, to));
        else
            return 0;
    }
    return 1;
}

static double mval_get_double(mval value, const char type)
{
    switch (type) {
        case 'f':
            return (double)value.f;
            break;
        case 'i':
            return (double)value.i32;
            break;
        case 'd':
            return value.d;
            break;
        default:
            return 0;
            break;
    }
}

int mapper_boundary_perform(mapper_connection connection,
                            mapper_signal_history_t *history)
{
    /* TODO: We are currently saving the processed values to output history.
     * it needs to be decided whether boundary processing should be inside the
     * feedback loop when past samples are called in expressions. */
    int i, muted = 0;
    double v[connection->props.dest_length];

    mval *dest_min_vector, *dest_max_vector;
    double dest_min, dest_max, total_range, difference, modulo_difference;
    mapper_boundary_action bound_min, bound_max;

    if (connection->props.bound_min == BA_NONE
        && connection->props.bound_max == BA_NONE)
    {
        return 1;
    }

    if (connection->props.dest_type == 'f') {
        float *vhistory = msig_history_value_pointer(*history);
        for (i = 0; i < history->length; i++)
            v[i] = (double)vhistory[i];
    }
    else if (connection->props.dest_type == 'i') {
        int *vhistory = msig_history_value_pointer(*history);
        for (i = 0; i < history->length; i++)
            v[i] = (double)vhistory[i];
    }
    else if (connection->props.dest_type == 'd') {
        double *vhistory = msig_history_value_pointer(*history);
        for (i = 0; i < history->length; i++)
            v[i] = vhistory[i];
    }
    else {
        trace("unknown type in mapper_boundary_perform()\n");
        return 0;
    }

    if (connection->props.range.known) {
        if (connection->props.range.dest_min <= connection->props.range.dest_max) {
            bound_min = connection->props.bound_min;
            bound_max = connection->props.bound_max;
            dest_min_vector = connection->props.range.dest_min;
            dest_max_vector = connection->props.range.dest_max;
        }
        else {
            bound_min = connection->props.bound_max;
            bound_max = connection->props.bound_min;
            dest_min_vector = connection->props.range.dest_max;
            dest_max_vector = connection->props.range.dest_min;
        }
        for (i = 0; i < history->length; i++) {
            dest_min = mval_get_double(dest_min_vector[i],
                                       connection->props.src_type);
            dest_max = mval_get_double(dest_max_vector[i],
                                       connection->props.dest_type);
            total_range = fabs(dest_max - dest_min);
            if (v[i] < dest_min) {
                switch (bound_min) {
                    case BA_MUTE:
                        // need to prevent value from being sent at all
                        muted = 1;
                        break;
                    case BA_CLAMP:
                        // clamp value to range minimum
                        v[i] = dest_min;
                        break;
                    case BA_FOLD:
                        // fold value around range minimum
                        difference = fabsf(v[i] - dest_min);
                        v[i] = dest_min + difference;
                        if (v[i] > dest_max) {
                            // value now exceeds range maximum!
                            switch (bound_max) {
                                case BA_MUTE:
                                    // need to prevent value from being sent at all
                                    muted = 1;
                                    break;
                                case BA_CLAMP:
                                    // clamp value to range minimum
                                    v[i] = dest_max;
                                    break;
                                case BA_FOLD:
                                    // both boundary actions are set to fold!
                                    difference = fabsf(v[i] - dest_max);
                                    modulo_difference = difference
                                        - ((int)(difference / total_range)
                                           * total_range);
                                    if ((int)(difference / total_range) % 2 == 0) {
                                        v[i] = dest_max - modulo_difference;
                                    }
                                    else
                                        v[i] = dest_min + modulo_difference;
                                    break;
                                case BA_WRAP:
                                    // wrap value back from range minimum
                                    difference = fabsf(v[i] - dest_max);
                                    modulo_difference = difference
                                        - ((int)(difference / total_range)
                                           * total_range);
                                    v[i] = dest_min + modulo_difference;
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case BA_WRAP:
                        // wrap value back from range maximum
                        difference = fabsf(v[i] - dest_min);
                        modulo_difference = difference
                            - (int)(difference / total_range) * total_range;
                        v[i] = dest_max - modulo_difference;
                        break;
                    default:
                        // leave the value unchanged
                        break;
                }
            }
            else if (v[i] > dest_max) {
                switch (bound_max) {
                    case BA_MUTE:
                        // need to prevent value from being sent at all
                        muted = 1;
                        break;
                    case BA_CLAMP:
                        // clamp value to range maximum
                        v[i] = dest_max;
                        break;
                    case BA_FOLD:
                        // fold value around range maximum
                        difference = fabsf(v[i] - dest_max);
                        v[i] = dest_max - difference;
                        if (v[i] < dest_min) {
                            // value now exceeds range minimum!
                            switch (bound_min) {
                                case BA_MUTE:
                                    // need to prevent value from being sent at all
                                    muted = 1;
                                    break;
                                case BA_CLAMP:
                                    // clamp value to range minimum
                                    v[i] = dest_min;
                                    break;
                                case BA_FOLD:
                                    // both boundary actions are set to fold!
                                    difference = fabsf(v[i] - dest_min);
                                    modulo_difference = difference
                                        - ((int)(difference / total_range)
                                           * total_range);
                                    if ((int)(difference / total_range) % 2 == 0) {
                                        v[i] = dest_max + modulo_difference;
                                    }
                                    else
                                        v[i] = dest_min - modulo_difference;
                                    break;
                                case BA_WRAP:
                                    // wrap value back from range maximum
                                    difference = fabsf(v[i] - dest_min);
                                    modulo_difference = difference
                                        - ((int)(difference / total_range)
                                           * total_range);
                                    v[i] = dest_max - modulo_difference;
                                    break;
                                default:
                                    break;
                            }
                        }
                        break;
                    case BA_WRAP:
                        // wrap value back from range minimum
                        difference = fabsf(v[i] - dest_max);
                        modulo_difference = difference
                            - (int)(difference / total_range) * total_range;
                        v[i] = dest_min + modulo_difference;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    if (connection->props.dest_type == 'f') {
        float *vhistory = msig_history_value_pointer(*history);
        for (i = 0; i < history->length; i++)
            vhistory[i] = (float)v[i];
    }
    else if (connection->props.dest_type == 'i') {
        int *vhistory = msig_history_value_pointer(*history);
        for (i = 0; i < history->length; i++)
            vhistory[i] = (int)v[i];
    }
    else if (connection->props.dest_type == 'd') {
        double *vhistory = msig_history_value_pointer(*history);
        for (i = 0; i < history->length; i++)
            vhistory[i] = v[i];
    }
    return !muted;
}

/* Helper to replace a connection's expression only if the given string
 * parses successfully. Returns 0 on success, non-zero on error. */
static int replace_expression_string(mapper_connection c,
                                     const char *expr_str,
                                     int *input_history_size,
                                     int *output_history_size)
{
    mapper_expr expr = mapper_expr_new_from_string(
        expr_str, c->props.src_type, c->props.dest_type,
        c->props.src_length, c->props.dest_length,
        input_history_size, output_history_size);

    if (!expr)
        return 1;

    if (c->expr)
        mapper_expr_free(c->expr);

    c->expr = expr;
    int len = strlen(expr_str);
    if (!c->props.expression || len > strlen(c->props.expression))
        c->props.expression = realloc(c->props.expression, len+1);

    /* Using strncpy() here causes memory profiling errors due to possible
     * overlapping memory (e.g. expr_str == c->props.expression). */
    memcpy(c->props.expression, expr_str, len);
    c->props.expression[len] = '\0';

    return 0;
}

void mapper_connection_set_mode_direct(mapper_connection c)
{
    c->props.mode = MO_BYPASS;
    reallocate_connection_histories(c, 1, 1);
}

void mapper_connection_set_mode_linear(mapper_connection c)
{
    char expr[256] = "";
    const char *e = expr;
    mapper_connection_range_t r = c->props.range;

    if (r.known
        & (CONNECTION_RANGE_SRC_MIN | CONNECTION_RANGE_SRC_MAX))
    {
        if (memcmp(r.src_min, r.src_max, sizeof(mval))==0) {
            if (c->props.src_type == 'f')
                snprintf(expr, 256, "y=%g", r.dest_min[0].f);
            else if (c->props.src_type == 'i')
                snprintf(expr, 256, "y=%i", r.dest_min[0].i32);
            else if (c->props.src_type == 'd')
                snprintf(expr, 256, "y=%g", r.dest_min[0].d);
        }

        else if (r.known == CONNECTION_RANGE_KNOWN
                 && mval_get_double(r.src_min[0], c->props.src_type)
                    == mval_get_double(r.dest_min[0], c->props.dest_type)
                 && mval_get_double(r.src_max[0], c->props.src_type)
                    == mval_get_double(r.dest_max[0], c->props.dest_type))
            snprintf(expr, 256, "y=x");

        else if (r.known == CONNECTION_RANGE_KNOWN) {
            double src_min = mval_get_double(r.src_min[0], c->props.src_type),
                   src_max = mval_get_double(r.src_max[0], c->props.src_type),
                   dest_min = mval_get_double(r.dest_min[0], c->props.dest_type),
                   dest_max = mval_get_double(r.dest_max[0], c->props.dest_type);

            double scale = ((dest_min - dest_max) / (src_min - src_max));

            double offset = ((dest_max * src_min - dest_min * src_max)
                             / (src_min - src_max));

            snprintf(expr, 256, "y=x*(%g)+(%g)", scale, offset);
        }
        else
            e = 0;
    }
    else
        e = 0;

    // If everything is successful, replace the connection's expression.
    if (e) {
        int input_history_size, output_history_size;
        if (!replace_expression_string(c, e, &input_history_size,
                                       &output_history_size)) {
            reallocate_connection_histories(c, 1, 1);
            c->props.mode = MO_LINEAR;
        }
    }
}

void mapper_connection_set_mode_expression(mapper_connection c,
                                           const char *expr)
{
    int input_history_size, output_history_size;
    if (replace_expression_string(c, expr, &input_history_size,
                                  &output_history_size))
        return;

    c->props.mode = MO_EXPRESSION;
    reallocate_connection_histories(c, input_history_size,
                                    output_history_size);
}

void mapper_connection_set_mode_reverse(mapper_connection c)
{
    c->props.mode = MO_REVERSE;
}

void mapper_connection_set_mode_calibrate(mapper_connection c)
{
    c->props.mode = MO_CALIBRATE;

    if (c->props.expression)
        free(c->props.expression);

    char expr[256];
    if (c->props.src_length == 1) {
        if (c->props.src_type == 'f')
            snprintf(expr, 256, "y=%g", c->props.range.dest_min[0].f);
        else if (c->props.src_type == 'i')
            snprintf(expr, 256, "y=%i", c->props.range.dest_min[0].i32);
        else if (c->props.src_type == 'd')
            snprintf(expr, 256, "y=%g", c->props.range.dest_min[0].d);
    }
    else
        snprintf(expr, 256, "y=destMin");
    c->props.expression = strdup(expr);

    c->calibrating = 0;
}

// Helper for setting mval from different lo_arg types
static int mval_set_from_lo_arg(mval *dest, const char dest_type,
                                lo_arg *src, const char src_type)
{
    if (dest_type == 'f') {
        if (src_type == 'f')        dest->f = src->f;
        else if (src_type == 'i')   dest->f = (float)src->i;
        else if (src_type == 'd')   dest->f = (float)src->d;
        else                        return 1;
    }
    else if (dest_type == 'i') {
        if (src_type == 'f')        dest->i32 = (int)src->f;
        else if (src_type == 'i')   dest->i32 = src->i;
        else if (src_type == 'd')   dest->i32 = (int)src->d;
        else                        return 1;
    }
    else if (dest_type == 'd') {
        if (src_type == 'f')        dest->d = (double)src->f;
        else if (src_type == 'i')   dest->d = (double)src->i;
        else if (src_type == 'd')   dest->d = src->d;
        else                        return 1;
    }
    return 0;
}

/* Helper to fill in the range (src_min, src_max, dest_min, dest_max)
 * based on message parameters and known connection and signal
 * properties; return flags to indicate which parts of the range were
 * found. */
static void set_range(mapper_connection c,
                      mapper_message_t *msg)
{
    lo_arg **args = NULL;
    const char *types = NULL;
    int i, length = 0;

    if (!c)
        return;

    /* The logic here is to first try to use information from the
     * message, starting with @srcMax, @srcMin, @destMax, @destMin,
     * and then @min and @max.
     * Next priority is already-known properties of the connection.
     * Lastly, we fill in source range from the signal. */

    /* @srcMax */
    args = mapper_msg_get_param(msg, AT_SRC_MAX);
    types = mapper_msg_get_type(msg, AT_SRC_MAX);
    length = mapper_msg_get_length(msg, AT_SRC_MAX);
    if (args && types) {
        if (length == c->props.src_length) {
            if (!c->props.range.src_max)
                c->props.range.src_max = (mval*) malloc(length * sizeof(mval));
            c->props.range.known |= CONNECTION_RANGE_SRC_MAX;
            for (i=0; i<length; i++) {
                if (mval_set_from_lo_arg(&c->props.range.src_max[i],
                                         c->props.src_type,
                                         args[i], types[i])) {
                    c->props.range.known &= ~CONNECTION_RANGE_SRC_MAX;
                    break;
                }
            }
        }
        else
            c->props.range.known &= ~CONNECTION_RANGE_SRC_MAX;
    }

    /* @srcMin */
    args = mapper_msg_get_param(msg, AT_SRC_MIN);
    types = mapper_msg_get_type(msg, AT_SRC_MIN);
    length = mapper_msg_get_length(msg, AT_SRC_MIN);
    if (args && types) {
        if (length == c->props.src_length) {
            if (!c->props.range.src_min)
                c->props.range.src_min = (mval*) malloc(length * sizeof(mval));
            c->props.range.known |= CONNECTION_RANGE_SRC_MIN;
            for (i=0; i<length; i++) {
                if (mval_set_from_lo_arg(&c->props.range.src_min[i],
                                         c->props.src_type,
                                         args[i], types[i])) {
                    c->props.range.known &= ~CONNECTION_RANGE_SRC_MIN;
                    break;
                }
            }
        }
        else
            c->props.range.known &= ~CONNECTION_RANGE_SRC_MIN;
    }

    /* @destMax */
    args = mapper_msg_get_param(msg, AT_DEST_MAX);
    types = mapper_msg_get_type(msg, AT_DEST_MAX);
    length = mapper_msg_get_length(msg, AT_DEST_MAX);
    if (args && types) {
        if (length == c->props.dest_length) {
            if (!c->props.range.dest_max)
                c->props.range.dest_max = (mval*) malloc(length * sizeof(mval));
            c->props.range.known |= CONNECTION_RANGE_DEST_MAX;
            for (i=0; i<length; i++) {
                if (mval_set_from_lo_arg(&c->props.range.dest_max[i],
                                         c->props.dest_type,
                                         args[i], types[i])) {
                    c->props.range.known &= ~CONNECTION_RANGE_DEST_MAX;
                    break;
                }
            }
        }
        else
            c->props.range.known &= ~CONNECTION_RANGE_DEST_MAX;
    }

    /* @destMin */
    args = mapper_msg_get_param(msg, AT_DEST_MIN);
    types = mapper_msg_get_type(msg, AT_DEST_MIN);
    length = mapper_msg_get_length(msg, AT_DEST_MIN);
    if (args && types) {
        if (length == c->props.dest_length) {
            if (!c->props.range.dest_min)
                c->props.range.dest_min = (mval*) malloc(length * sizeof(mval));
            c->props.range.known |= CONNECTION_RANGE_DEST_MIN;
            for (i=0; i<length; i++) {
                if (mval_set_from_lo_arg(&c->props.range.dest_min[i],
                                         c->props.dest_type,
                                         args[i], types[i])) {
                    c->props.range.known &= ~CONNECTION_RANGE_DEST_MIN;
                    break;
                }
            }
        }
        else
            c->props.range.known &= ~CONNECTION_RANGE_DEST_MIN;
    }

    /* @min, @max */
    args = mapper_msg_get_param(msg, AT_MIN);
    types = mapper_msg_get_type(msg, AT_MIN);
    length = mapper_msg_get_length(msg, AT_MIN);
    if (!(c->props.range.known & CONNECTION_RANGE_DEST_MIN)
        && args && types)
    {
        if (length == c->props.dest_length) {
            if (!c->props.range.dest_min)
                c->props.range.dest_min = (mval*) malloc(length * sizeof(mval));
            c->props.range.known |= CONNECTION_RANGE_DEST_MIN;
            for (i=0; i<length; i++) {
                if (mval_set_from_lo_arg(&c->props.range.dest_min[i],
                                         c->props.dest_type,
                                         args[i], types[i])) {
                    c->props.range.known &= ~CONNECTION_RANGE_DEST_MIN;
                    break;
                }
            }
        }
        else
            c->props.range.known &= ~CONNECTION_RANGE_DEST_MIN;
    }

    args = mapper_msg_get_param(msg, AT_MAX);
    types = mapper_msg_get_type(msg, AT_MAX);
    length = mapper_msg_get_length(msg, AT_MAX);
    if (!(c->props.range.known & CONNECTION_RANGE_DEST_MAX)
        && args && types)
    {
        if (length == c->props.dest_length) {
            if (!c->props.range.dest_max)
                c->props.range.dest_max = (mval*) malloc(length * sizeof(mval));
            c->props.range.known |= CONNECTION_RANGE_DEST_MAX;
            for (i=0; i<length; i++) {
                if (mval_set_from_lo_arg(&c->props.range.dest_max[i],
                                         c->props.dest_type,
                                         args[i], types[i])) {
                    c->props.range.known &= ~CONNECTION_RANGE_DEST_MAX;
                    break;
                }
            }
        }
        else
            c->props.range.known &= ~CONNECTION_RANGE_DEST_MAX;
    }

    /* Signal */
    mapper_signal sig = c->parent->signal;

    /* If parent signal is an output it must be the "source" of this connection,
     * if it is an input it must be the "destination". According to the protocol
     * for negotiating new connections, we will only fill-in ranges for the
     * "source" signal.*/
    if (!sig || !sig->props.is_output)
        return;

    if (!c->props.range.src_min && sig->props.minimum)
    {
        c->props.range.src_min = (mval*) malloc(sig->props.length * sizeof(mval));
        memcpy(c->props.range.src_min, sig->props.minimum,
               sig->props.length * sizeof(mval));
        c->props.range.known |= CONNECTION_RANGE_SRC_MIN;
    }

    if (!c->props.range.src_max && sig->props.maximum)
    {
        c->props.range.src_max = (mval*) malloc(sig->props.length * sizeof(mval));
        memcpy(c->props.range.src_max, sig->props.maximum,
               sig->props.length * sizeof(mval));
        c->props.range.known |= CONNECTION_RANGE_SRC_MAX;
    }
}

void mapper_connection_set_from_message(mapper_connection c,
                                        mapper_message_t *msg)
{
    /* First record any provided parameters. */

    /* Destination type. */

    const char *dest_type = mapper_msg_get_param_if_char(msg, AT_TYPE);
    if (dest_type)
        c->props.dest_type = dest_type[0];

    /* Range information. */

    set_range(c, msg);
    if (c->props.range.known == CONNECTION_RANGE_KNOWN &&
        c->props.mode == MO_LINEAR) {
        mapper_connection_set_mode_linear(c);
    }

    /* Muting. */
    int muting;
    if (!mapper_msg_get_param_if_int(msg, AT_MUTE, &muting))
        c->props.muted = muting;

    /* Range boundary actions. */
    int bound_min = mapper_msg_get_boundary_action(msg, AT_BOUND_MIN);
    if (bound_min >= 0)
        c->props.bound_min = bound_min;

    int bound_max = mapper_msg_get_boundary_action(msg, AT_BOUND_MAX);
    if (bound_max >= 0)
        c->props.bound_max = bound_max;

    /* Expression. */
    const char *expr = mapper_msg_get_param_if_string(msg, AT_EXPRESSION);
    if (expr) {
        int input_history_size, output_history_size;
        if (!replace_expression_string(c, expr, &input_history_size,
                                       &output_history_size)) {
            if (c->props.mode == MO_EXPRESSION)
                reallocate_connection_histories(c, input_history_size,
                                                output_history_size);
        }
    }

    /* Instances. */
    int send_as_instance;
    if (!mapper_msg_get_param_if_int(msg, AT_SEND_AS_INSTANCE, &send_as_instance))
        c->props.send_as_instance = send_as_instance;

    /* Extra properties. */
    mapper_msg_add_or_update_extra_params(c->props.extra, msg);

    /* Now set the mode type depending on the requested type and
     * the known properties. */

    int mode = mapper_msg_get_mode(msg);

    switch (mode)
    {
    case -1:
        /* No mode type specified; if mode not yet set, see if
         we know the range and choose between linear or direct connection. */
            if (c->props.mode == MO_UNDEFINED) {
                if (c->props.range.known == CONNECTION_RANGE_KNOWN) {
                    /* We have enough information for a linear connection. */
                    mapper_connection_set_mode_linear(c);
                } else
                    /* No range, default to direct connection. */
                    mapper_connection_set_mode_direct(c);
            }
        break;
    case MO_BYPASS:
        mapper_connection_set_mode_direct(c);
        break;
    case MO_LINEAR:
        if (c->props.range.known == CONNECTION_RANGE_KNOWN) {
            mapper_connection_set_mode_linear(c);
        }
        break;
    case MO_CALIBRATE:
        if (c->props.range.known & (CONNECTION_RANGE_DEST_MIN
                                    | CONNECTION_RANGE_DEST_MAX))
            mapper_connection_set_mode_calibrate(c);
        break;
    case MO_EXPRESSION:
        {
            if (!c->props.expression)
                c->props.expression = strdup("y=x");
            mapper_connection_set_mode_expression(c, c->props.expression);
        }
        break;
    case MO_REVERSE:
        mapper_connection_set_mode_reverse(c);
        break;
    default:
        trace("unknown result from mapper_msg_get_mode()\n");
        break;
    }
}

mapper_connection mapper_connection_find_by_names(mapper_device md,
                                                  const char* src_name,
                                                  const char* dest_name)
{
    mapper_router router = md->routers;
    int i = 0;
    int n = strlen(dest_name);
    const char *slash = strchr(dest_name+1, '/');
    if (slash)
        n = n - strlen(slash);

    src_name = strchr(src_name+1, '/');

    while (i < md->props.n_outputs) {
        // Check if device outputs includes src_name
        if (strcmp(md->outputs[i]->props.name, src_name) == 0) {
            while (router != NULL) {
                // find associated router
                if (strncmp(router->props.dest_name, dest_name, n) == 0) {
                    // find associated connection
                    mapper_router_signal rs = router->signals;
                    while (rs && rs->signal != md->outputs[i])
                        rs = rs->next;
                    if (!rs)
                        return NULL;
                    mapper_connection c = rs->connections;
                    while (c && strcmp(c->props.dest_name,
                                       (dest_name + n)) != 0)
                        c = c->next;
                    if (!c)
                        return NULL;
                    else
                        return c;
                }
                else {
                    router = router->next;
                }
            }
            return NULL;
        }
        i++;
    }
    return NULL;
}

void reallocate_connection_histories(mapper_connection c,
                                     int input_history_size,
                                     int output_history_size)
{
    mapper_signal sig = c->parent->signal;
    int i;

    // At least for now, exit if this is an input signal
    if (!sig->props.is_output)
        return;

    // If there is no expression, then no memory needs to be
    // reallocated.
    if (!c->expr)
        return;

    if (input_history_size < 1)
        input_history_size = 1;

    if (input_history_size > sig->props.history_size) {
        int sample_size = msig_vector_bytes(sig);
        for (i=0; i<sig->props.num_instances; i++) {
            mhist_realloc(&c->parent->history[i], input_history_size,
                          sample_size, 0);
        }
        sig->props.history_size = input_history_size;
    }
    else if (input_history_size < sig->props.history_size) {
        // Do nothing for now...
        // Find maximum input length needed for connections
        /*mapper_connection temp = c->parent->connections;
        while (c) {
            if (c->props.mode == MO_EXPRESSION) {
                if (c->expr->input_history_size > input_history_size) {
                    input_history_size = c->expr->input_history_size;
                }
            }
            c = c->next;
        }*/
    }
    if (output_history_size > c->props.dest_history_size) {
        int sample_size = mapper_type_size(c->props.dest_type) * c->props.dest_length;
        for (i=0; i<sig->props.num_instances; i++) {
            mhist_realloc(&c->history[i], output_history_size, sample_size, 1);
        }
        c->props.dest_history_size = output_history_size;
    }
    else if (output_history_size < mapper_expr_output_history_size(c->expr)) {
        // Do nothing for now...
    }
}

void mhist_realloc(mapper_signal_history_t *history,
                   int history_size,
                   int sample_size,
                   int is_output)
{
    if (!history || !history_size || !sample_size)
        return;
    if (history_size == history->size)
        return;
    if (is_output || (history_size > history->size) || (history->position == 0)) {
        // realloc in place
        history->value = realloc(history->value, history_size * sample_size);
        history->timetag = realloc(history->timetag, history_size * sizeof(mapper_timetag_t));
        if (is_output) {
            // Initialize entire history to 0
            memset(history->value, 0, history_size * sample_size);
            history->position = -1;
        }
        else if (history->position == 0) {
            memset(history->value + sample_size * history->size, 0,
                   sample_size * (history_size - history->size));
        }
        else {
            int new_position = history_size - history->size + history->position;
            memcpy(history->value + sample_size * new_position,
                   history->value + sample_size * history->position,
                   sample_size * (history->size - history->position));
            memcpy(&history->timetag[new_position],
                   &history->timetag[history->position], sizeof(mapper_timetag_t)
                   * (history->size - history->position));
            memset(history->value + sample_size * history->position, 0,
                   sample_size * (history_size - history->size));
        }
    }
    else {
        // copying into smaller array
        if (history->position >= history_size * 2) {
            // no overlap - memcpy ok
            int new_position = history_size - history->size + history->position;
            memcpy(history->value,
                   history->value + sample_size * (new_position - history_size),
                   sample_size * history_size);
            memcpy(&history->timetag,
                   &history->timetag[history->position - history_size],
                   sizeof(mapper_timetag_t) * history_size);
            history->value = realloc(history->value, history_size * sample_size);
            history->timetag = realloc(history->timetag, history_size * sizeof(mapper_timetag_t));
        }
        else {
            // there is overlap between new and old arrays - need to allocate new memory
            mapper_signal_history_t temp;
            temp.value = malloc(sample_size * history_size);
            temp.timetag = malloc(sizeof(mapper_timetag_t) * history_size);
            if (history->position < history_size) {
                memcpy(temp.value, history->value,
                       sample_size * history->position);
                memcpy(temp.value + sample_size * history->position,
                       history->value + sample_size
                       * (history->size - history_size + history->position),
                       sample_size * (history_size - history->position));
                memcpy(temp.timetag, history->timetag,
                       sizeof(mapper_timetag_t) * history->position);
                memcpy(&temp.timetag[history->position],
                       &history->timetag[history->size - history_size + history->position],
                       sizeof(mapper_timetag_t) * (history_size - history->position));
            }
            else {
                memcpy(temp.value, history->value + sample_size
                       * (history->position - history_size),
                       sample_size * history_size);
                memcpy(temp.timetag,
                       &history->timetag[history->position - history_size],
                       sizeof(mapper_timetag_t) * history_size);
                history->position = history_size - 1;
            }
            free(history->value);
            free(history->timetag);
            history->value = temp.value;
            history->timetag = temp.timetag;
        }
    }
    history->size = history_size;
}
