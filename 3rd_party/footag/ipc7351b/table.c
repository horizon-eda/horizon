/* IPC-7351B solder joint goals for different lead types. */

#include "priv.h"

#define DENS_M IPCB_DENSITY_M
#define DENS_N IPCB_DENSITY_N
#define DENS_L IPCB_DENSITY_L

const struct ipcbtable ipcb_table3_2 = {
        .ref    = "Table 3-2",
        .desc   = "Flat Ribbon L and Gull-Wing Leads "
                  "(greater than 0.625 mm pitch)",
        .toe    = { [DENS_M] =  0.55, [DENS_N] =  0.35, [DENS_L] =  0.15 },
        .heel   = { [DENS_M] =  0.45, [DENS_N] =  0.35, [DENS_L] =  0.25 },
        .side   = { [DENS_M] =  0.05, [DENS_N] =  0.03, [DENS_L] =  0.01 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_3 = {
        .ref    = "Table 3-3",
        .desc   = "Flat Ribbon L and Gull-Wing Leads "
                  "(less than or equal to 0.625 mm pitch)",
        .toe    = { [DENS_M] =  0.55, [DENS_N] =  0.35, [DENS_L] =  0.15 },
        .heel   = { [DENS_M] =  0.45, [DENS_N] =  0.35, [DENS_L] =  0.25 },
        .side   = { [DENS_M] =  0.01, [DENS_N] = -0.02, [DENS_L] = -0.04 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_4 = {
        .ref    = "Table 3-4",
        .desc   = "J Leads",
        .heel   = { [DENS_M] =  0.55, [DENS_N] =  0.35, [DENS_L] =  0.15 },
        .toe    = { [DENS_M] =  0.10, [DENS_N] =  0.00, [DENS_L] =  -0.10 },
        .side   = { [DENS_M] =  0.05, [DENS_N] =  0.03, [DENS_L] =  0.01 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_5 = {
        .ref    = "Table 3-5",
        .desc   = "Rectangular or Square-End Components (Capacitors and "
                  "Resistors) Equal to or Larger than 1608 (0603)",
        .toe    = { [DENS_M] =  0.55, [DENS_N] =  0.35, [DENS_L] =  0.15 },
        .heel   = { [DENS_M] =  0.00, [DENS_N] =  0.00, [DENS_L] =  0.00 },
        .side   = { [DENS_M] =  0.05, [DENS_N] =  0.00, [DENS_L] = -0.05 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_6 = {
        .ref    = "Table 3-6",
        .desc   = "Rectangular or Square-End Components (Capacitors and "
                  "Resistors) Smaller than 1608 (0603)",
        .toe    = { [DENS_M] =  0.30, [DENS_N] =  0.20, [DENS_L] =  0.10 },
        .heel   = { [DENS_M] =  0.00, [DENS_N] =  0.00, [DENS_L] =  0.00 },
        .side   = { [DENS_M] =  0.05, [DENS_N] =  0.00, [DENS_L] = -0.05 },
        .round  = 0.02,
        .cyexc  = { [DENS_M] =  0.20, [DENS_N] =  0.15, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_7 = {
        .ref    = "Table 3-7",
        .desc   = "Cylindrical End Cap Terminations (MELF)",
        .toe    = { [DENS_M] =  0.60, [DENS_N] =  0.40, [DENS_L] =  0.20 },
        .heel   = { [DENS_M] =  0.20, [DENS_N] =  0.10, [DENS_L] =  0.02 },
        .side   = { [DENS_M] =  0.10, [DENS_N] =  0.05, [DENS_L] =  0.01 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_9 = {
        .ref    = "Table 3-9",
        .desc   = "Concave Chip Array Component Lead Package",
        .toe    = { [DENS_M] =  0.55, [DENS_N] =  0.45, [DENS_L] =  0.35 },
        .heel   = { [DENS_M] = -0.05, [DENS_N] = -0.07, [DENS_L] = -0.10 },
        .side   = { [DENS_M] = -0.05, [DENS_N] = -0.07, [DENS_L] = -0.10 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_10 = {
        .ref    = "Table 3-10",
        .desc   = "Convex Chip Array Component Lead Package",
        .toe    = { [DENS_M] =  0.55, [DENS_N] =  0.45, [DENS_L] =  0.35 },
        .heel   = { [DENS_M] = -0.05, [DENS_N] = -0.07, [DENS_L] = -0.10 },
        .side   = { [DENS_M] = -0.05, [DENS_N] = -0.07, [DENS_L] = -0.10 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_11 = {
        .ref    = "Table 3-11",
        .desc   = "Flat Chip Array Component Lead Package",
        .toe    = { [DENS_M] =  0.55, [DENS_N] =  0.45, [DENS_L] =  0.35 },
        .heel   = { [DENS_M] = -0.05, [DENS_N] = -0.07, [DENS_L] = -0.10 },
        .side   = { [DENS_M] = -0.05, [DENS_N] = -0.07, [DENS_L] = -0.10 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_13 = {
        .ref    = "Table 3-13",
        .desc   = "Inward Flat Ribbon L-Leads (Molded Inductors, Diodes & "
                  "Polarized Capacitors)",
        .toe    = { [DENS_M] =  0.25, [DENS_N] =  0.15, [DENS_L] =  0.07 },
        .heel   = { [DENS_M] =  0.80, [DENS_N] =  0.50, [DENS_L] =  0.20 },
        .side   = { [DENS_M] =  0.01, [DENS_N] = -0.05, [DENS_L] = -0.10 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_15 = {
        .ref    = "Table 3-15",
        .desc   = "Quad Flat No-Lead",
        .toe    = { [DENS_M] =  0.40, [DENS_N] =  0.30, [DENS_L] =  0.20 },
        .heel   = { [DENS_M] =  0.00, [DENS_N] =  0.00, [DENS_L] =  0.00 },
        .side   = { [DENS_M] = -0.04, [DENS_N] = -0.04, [DENS_L] = -0.04 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_16 = {
        .ref    = "Table 3-16",
        .desc   = "Small Outline No-Lead",
        .toe    = { [DENS_M] =  0.40, [DENS_N] =  0.30, [DENS_L] =  0.20 },
        .heel   = { [DENS_M] =  0.00, [DENS_N] =  0.00, [DENS_L] =  0.00 },
        .side   = { [DENS_M] = -0.04, [DENS_N] = -0.04, [DENS_L] = -0.04 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_18 = {
        .ref    = "Table 3-18",
        .desc   = "Small Outline and Quad Flat No-Lead with Pullback Leads",
        .toe    = { [DENS_M] =  0.05, [DENS_N] =  0.00, [DENS_L] = -0.05 },
        .heel   = { [DENS_M] =  0.05, [DENS_N] =  0.00, [DENS_L] = -0.05 },
        .side   = { [DENS_M] =  0.05, [DENS_N] =  0.00, [DENS_L] = -0.05 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.50, [DENS_N] =  0.25, [DENS_L] =  0.10 },
};

const struct ipcbtable ipcb_table3_20 = {
        .ref    = "Table 3-20",
        .desc   = "Aluminium Electrolytic Capacitor and 2-pin Crystal",
        .toe    = { [DENS_M] =  0.70, [DENS_N] =  0.50, [DENS_L] =  0.30 },
        .heel   = { [DENS_M] =  0.00, [DENS_N] = -0.10, [DENS_L] = -0.20 },
        .side   = { [DENS_M] =  0.50, [DENS_N] =  0.40, [DENS_L] =  0.30 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  1.00, [DENS_N] =  0.50, [DENS_L] =  0.25 },
};

const struct ipcbtable ipcb_table3_22 = {
        .ref    = "Table 3-22",
        .desc   = "Small Outline Components, Flat Lead",
        .toe    = { [DENS_M] =  0.30, [DENS_N] =  0.20, [DENS_L] =  0.10 },
        .heel   = { [DENS_M] =  0.00, [DENS_N] =  0.00, [DENS_L] =  0.00 },
        .side   = { [DENS_M] =  0.05, [DENS_N] =  0.00, [DENS_L] = -0.05 },
        .round  = 0.05,
        .cyexc  = { [DENS_M] =  0.20, [DENS_N] =  0.15, [DENS_L] =  0.10 },
};

#undef DENS_M
#undef DENS_N
#undef DENS_L

