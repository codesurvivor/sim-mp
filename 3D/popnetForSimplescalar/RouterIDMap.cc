#include "RouterIDMap.h"
#include "configuration.h"
vector<MappingItem> MappingTable;
void FindNetworkID(add_type &Proc, add_type & Network)
{
	int i ;
	for ( i = 0 ; i <MappingTable.size(); i++ )
	{
		if (MappingTable[i].Proc[0] == Proc[0] && MappingTable[i].Proc[1] == Proc[1])
		{
			break;
		}
	}
	if (i ==MappingTable.size())
	{
		printf("cannot come here");
		assert(0);
	}
	Network =  MappingTable[i].Network;
}
void FindProcID(add_type &Proc, add_type & Network)
{
	int i ;
	for ( i = 0 ; i <MappingTable.size(); i++ )
	{
		if (MappingTable[i].Network[0] == Network[0] && MappingTable[i].Network[1] == Network[1])
		{
			break;
		}
	}
	if (i ==MappingTable.size())
	{
		printf("cannot come here");
		assert(0);
	}
	Proc = MappingTable[i].Proc;
}
void Generate3DMeshProcNetworkID()
{
	MappingItem m;
	int network_x, network_y;
	int new_x, new_y, new_z ;
	int NetworkID;
	int ary_size = mesh_size+2;
	int MC_index = 0;
	// print it out
	ofstream db;
	db.open("db.log");
	for	(int i = 0; i < mesh_size+2; i ++)
	{
		for (int j= 0;j <mesh_size;j++)
		{
			if (i > 0 && i < mesh_size +MEM_LOC_SHIFT)
			{
				// normal nodes, to 3D mesh
				int ProcID = (i - MEM_LOC_SHIFT)*mesh_size+j;

				new_z = ProcID / ( DIM_X*DIM_Y) +1 ; // let the bottom layer be the memory 
				new_y= (ProcID % (DIM_X*DIM_Y) )/ DIM_X;
				new_x = (ProcID % (DIM_X*DIM_Y) )% DIM_X;
				
				NetworkID = new_z* DIM_X*DIM_Y + new_y*DIM_X+new_x;
				network_y = NetworkID / ary_size;
				network_x = NetworkID % ary_size;	
	   			db<<"( " <<i<<", "<<j<<" ) "
				<<ProcID<<" Network ( "<<new_x<<", "<<new_y<<", "<<new_z<<") "<<NetworkID
				<<" ( " <<network_y<<", "<<network_x<<" )\n";		
			}		
			else 
			{
				// This is a mem controller, we map them to the bottom line
					new_z = 0; 
					//if (i == 0)
					//{
					//	new_y = 0;
					//}
					//else 
					//{
					//	new_y = DIM_Y-1;
					//}
					//int delta = mesh_size/DIM_X ;
					//new_x = j/ delta;
					NetworkID = MC_index++;
					new_y = (NetworkID %(DIM_X*DIM_Y)) /DIM_X;
					new_x = (NetworkID %(DIM_X*DIM_Y)) %DIM_X;
					//new_z* DIM_X*DIM_Y + new_y*DIM_X+new_x;
					network_y = NetworkID / ary_size;
					network_x = NetworkID % ary_size;
	   				db<<"( " <<i<<", "<<j<<" ) "
					<<-1<<" Network ( "<<new_x<<", "<<new_y<<", "<<new_z<<") "<<NetworkID
					<<" ( " <<network_y<<", "<<network_x<<" )\n";				
			}
			m.Proc.resize(2);
			m.Network.resize(2);
			m.Proc[0] = i;
			m.Proc[1] = j;
			m.Network[0] = network_y;
			m.Network[1] = network_x;
			MappingTable.push_back(m);
		}
	}
	  
	db.close();
}

int ConnectionDownStream(int i, add_type & address_, add_type & next_add_t, long & next_pc_t, int ary_size_) // Credit
{
	long next_add_z, next_add_y, next_add_x;
	long local_id=address_[0]*ary_size_+address_[1];
	next_add_z = local_id / ( DIM_X*DIM_Y)  ;
	next_add_y = (local_id % (DIM_X*DIM_Y) )/ DIM_X;
	next_add_x = (local_id % (DIM_X*DIM_Y) )% DIM_X;

	switch (Topo)
	{
		case ThreeD_MESH:
		if((i%2)==0)
		{
				next_pc_t=i-1;
				switch(i) 
				{
					case 2 : next_add_x++; break;
					case 4 : next_add_y++; break;
					case 6 : next_add_z++; break;
					default: break;
				}
			}
			else 
			{
				next_pc_t=i+1;
				switch(i) 
				{
					case 1 : next_add_x--; break;
					case 3 : next_add_y--; break;
					case 5 : next_add_z--; break;
					default: break;
				}
			}
		break;
	}
	int upstream_ID = next_add_z* DIM_X*DIM_Y + next_add_y*DIM_X+next_add_x;
	next_add_t[0]=upstream_ID/ary_size_;
	next_add_t[1]=upstream_ID%ary_size_;
}
