#ifndef ROUTERIDMAP_H
#define ROUTERIDMAP_H
#include <vector>
#include <fstream>
#include "assert.h"
#include "index.h"
using namespace std;
extern int DIM_X, DIM_Y, DIM_Z;
extern int AFNCubeSize, AFNCubeNum;
extern int mesh_size;
extern int mem_port_num;
enum TOPO 
{ TwoD_MESH, TwoD_TORUS, TwoD_FT, ThreeD_MESH, ThreeD_CMESH, AFN_O, AFN_E, ThreeD_MECS};
extern int Topo;

enum BYPASS
{ EVC, VIP, NONE};
extern int BypassScheme;

#define MEM_LOC_SHIFT 1
extern void FindNetworkID(add_type &Proc, add_type & Network);
extern void FindProcID(add_type &Proc, add_type & Network);
extern void Generate3DMeshProcNetworkID();
extern int ConnectionDownStream(int i, add_type & address_, add_type & next_add_t, long & next_pc_t, int ary_size_) ;

struct MappingItem
{
	add_type Proc;
	add_type Network;
};
extern vector<MappingItem> MappingTable;

#endif
