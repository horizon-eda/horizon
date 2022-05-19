#pragma once

enum class VIATYPE : int
{
    THROUGH      = 3, /* Always a through hole via */
    BLIND_BURIED = 2, /* this via can be on internal layers */
    MICROVIA     = 1, /* this via which connect from an external layer
                                * to the near neighbor internal layer */
    NOT_DEFINED  = 0  /* not yet used */
};
