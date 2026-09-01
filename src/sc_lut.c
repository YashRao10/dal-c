/*
 * sc_lut.c -- implementation. Blocks are tagged with the low-level
 * requirement they satisfy; see requirements/sc_lut.md.
 */
#include "sc_lut.h"
#include "sc_sat.h"

sc_status_t sc_lut_valid(const sc_lut_t *lut)
{
    sc_status_t status = SC_OK;

    if ((lut == NULL) || (lut->x == NULL) || (lut->y == NULL))
    {
        status = SC_ERR_NULL;                        /* LLR-LUT-8 */
    }
    else if (lut->n < 2u)
    {
        status = SC_ERR_PARAM;                       /* LLR-LUT-9 */
    }
    else
    {
        uint32_t i;
        for (i = 0u; (i + 1u) < lut->n; i++)         /* LLR-LUT-10 */
        {
            if (lut->x[i + 1u] <= lut->x[i])
            {
                status = SC_ERR_PARAM;               /* not strictly increasing */
            }
        }
    }

    return status;
}

int32_t sc_lut_eval(const sc_lut_t *lut, int32_t x)
{
    int32_t result;

    if (lut == NULL)
    {
        result = 0;                                  /* LLR-LUT-7 */
    }
    else if (x <= lut->x[0])
    {
        result = lut->y[0];                          /* LLR-LUT-1: clamp low */
    }
    else
    {
        uint32_t i = 0u;

        while ((i + 1u) < lut->n)                    /* LLR-LUT-3: find segment */
        {
            if (x < lut->x[i + 1u])
            {
                break;
            }
            i++;
        }

        if ((i + 1u) >= lut->n)
        {
            result = lut->y[lut->n - 1u];            /* LLR-LUT-2: clamp high */
        }
        else
        {
            int64_t span   = (int64_t)lut->x[i + 1u] - (int64_t)lut->x[i]; /* > 0 */
            int64_t step   = (int64_t)lut->y[i + 1u] - (int64_t)lut->y[i]; /* LLR-LUT-4 */
            int64_t offset = (step * ((int64_t)x - (int64_t)lut->x[i])) / span; /* LLR-LUT-5 */

            result = sc_sat_add_i32(lut->y[i], (int32_t)offset);          /* LLR-LUT-6 */
        }
    }

    return result;
}
