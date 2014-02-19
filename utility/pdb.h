#ifndef pdb_h_
#define pdb_h_

// Multiple input & output objects use the Programmable Delay Block
// to set their sample rate.  They must all configure the same
// period to avoid chaos.

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT)
#define PDB_PERIOD 1087 // 48e6 / 44100

#endif
