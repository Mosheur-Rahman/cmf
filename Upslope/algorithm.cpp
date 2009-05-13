#include "algorithm.h"
#include "Topology.h"
#include "SoilWaterStorage.h"
cmf::upslope::Cell* cmf::upslope::find_cell(cmf::upslope::cells_ref cells,geometry::point p,double max_dist )
{
	double min_dist=max_dist;
	cmf::upslope::Cell* res;
	for(cmf::upslope::cell_vector::const_iterator it = cells.begin(); it != cells.end(); ++it)
	{
		double dist=p.distanceTo((*it)->Center());
		if (dist<min_dist)
		{
			min_dist=dist;
			res=*it;
		}
	}
	return res;
}
cmf::upslope::cell_vector cmf::upslope::boundary_cells(cmf::upslope::cells_ref cells)
{
	using namespace cmf;
	using namespace cmf::upslope;
	cell_vector resVector;
	if (cells.size()==0) return resVector;
	//Build a set of upslope cells from Upslope() for faster random access
	cell_set upcSet(cells.begin(),cells.end());

	//Find a cell to start. We use the most western one, which has to be a boundary cell
	Cell* startCell=cells[0];
	for (cell_vector::const_iterator it=cells.begin();it!=cells.end();it++)
		if ((*it)->Center().x < startCell->Center().x)
			startCell=*it;

	//From this start cell we are looking for the next neighbor in clockwise direction
	Cell* nextCell=0;
	Cell* thisCell=0;
	//until we are again at the start
	while (startCell!=nextCell)
	{
		//remember the last cell
		Cell* lastCell=thisCell;
		//if the "next" cell is not definied use the start cell
		thisCell=nextCell ? nextCell : startCell;
		//remember the current cell
		resVector.push_back(thisCell);
		//the azimuth to the cell from where we are coming (or 0 if it's the first loop)
		double azimuthToLast=lastCell ? thisCell->Center().azimuth(lastCell->Center()) : 270;
		//We are looking for the next cell clockwise
		double minAngleDiff=361;
		nextCell=0;
		//Find the next cell in the neighborhood in clockwise direction
		for (NeighborIterator neighbor = thisCell; neighbor.valid(); ++neighbor)
		{
			// If the connection to the neighbor is active and element of this
			if ((upcSet.find(neighbor->cell)!=upcSet.end()))
			{																						
				double angleDiff=	//the difference angle (in clockwise direction) at thisCell between the last cell and one of the neighbors
					thisCell->Center().azimuth(neighbor->Center()) //Azimuth to one of the neighbors
					- azimuthToLast;																	//Minus the Azimuth to the lastCell (0 at start cell)
				//Since neighbors in counter clockwise direction may be negative, we have to solve negative angles (e.g. -90deg->270deg)
				if (angleDiff<=0) angleDiff+=360;
				if (angleDiff<minAngleDiff)
				{
					minAngleDiff=angleDiff;
					nextCell=&(neighbor.cell());
				}
			}
		}
	}
	return resVector;
}


void cmf::upslope::connect_cells_with_flux(cmf::upslope::cells_ref cells, const cmf::upslope::CellConnector& connect,int start_at_layer)
{
	cmf::upslope::cell_set cs(cells.begin(),cells.end());
	while (cs.size())
	{
		cmf::upslope::Cell& cell=**cs.begin();
		cs.erase(&cell);
		for (cmf::upslope::NeighborIterator it=cell;it.valid();++it)
			if (cs.find(&it.cell())!=cs.end())
				connect(cell,it.cell(),start_at_layer);
	}
}
void insert_connections_in_set(std::set<cmf::water::FluxConnection*>& cset,cmf::water::FluxNode* node)
{
	cmf::water::connection_vector cons=node->Connections();
	for(cmf::water::connection_vector::const_iterator it = cons.begin(); it != cons.end(); ++it)
	{
		cset.insert(*it);
	}
}
cmf::water::connection_set cmf::upslope::get_connections(cmf::upslope::cells_ref cells)
{
	using namespace cmf::water;
	std::set<FluxConnection*> cset;
	//insert_connections_in_set(cset,&Rainfall());
	for(cmf::upslope::cell_vector::const_iterator it = cells.begin(); it != cells.end(); ++it)
	{
		cmf::upslope::Cell& c=**it;
		//insert_connections_in_set(cset,&c.Evaporation());
		insert_connections_in_set(cset,&c.SurfaceWater());
		for (int i = 0; i < c.StorageCount(); ++i)
		{
			insert_connections_in_set(cset,&c.GetStorage(i));
		}
		for (int i = 0; i < c.LayerCount() ; ++i)
		{
			insert_connections_in_set(cset,&c.Layer(i));
		}		 
	}
	return cset;
}

double cmf::upslope::area( cmf::upslope::cells_ref cells )
{
	double sum=0;
	for(cmf::upslope::cell_vector::const_iterator it = cells.begin(); it != cells.end(); ++it)
	{
		sum+=(*it)->Area();
	}
	return sum;
}

int cmf::upslope::fill_sinks( cells_ref cells,double min_difference/*=0.001*/ )
{
	int NoOfActions=0;
	cell_vector boundary=boundary_cells(cells);
	/// A set of boundary cells, (set::find is faster than vector::find) needed for the alghorithm
	std::set<Cell*> boundarySet=std::set<Cell*>(boundary.begin(),boundary.end());


	//Fill sinks according to 
	//Planchon and Darboux, 2002 O. Planchon and F. Darboux, 
	//A fast, simple and versatile algorithm to fill the depressions of digital elevation models, 
  //Catena 46 (2002), pp. 159176.

	//We need to remember the old height
	std::map<Cell*,double>& oldHeightMap=*new std::map<Cell*,double>;

	//Step 1: Fill the whole DEM with water (without the boundaries)
	for (cell_vector::const_iterator it=cells.begin();it!=cells.end();it++)
	{
		if (boundarySet.find(*it)==boundarySet.end())
		{
			oldHeightMap.insert(std::make_pair(*it,(*it)->z));
			(*it)->z=1e17;
		}
	}
	//Step 2: Drain the water, until the sinks are filled
	const double e=min_difference;
	bool continueIncrementation=true;
	while (continueIncrementation)
	{
		continueIncrementation=false;
		//Cycle through all cells
		for (std::map<Cell*,double>::iterator it=oldHeightMap.begin();it!=oldHeightMap.end();it++)
		{
			//Reference variable, its just an alias for the referenced variables, not a memory position itself
			double& 
				Wc=it->first->z,
				Zc=it->second;

			if (Wc > Zc)
			{
				//Loop through the neighbors end exit if operation 1 was performed
				for (NeighborIterator n_iter(it->first); n_iter.valid() ; ++n_iter)
				{
					double Wn=n_iter->z;
					//Operation 1 applicable?
					if (Zc>=Wn + e)
					{
						//Do operation 1 (If there is a lower new height than the original height of the cell in the neighborhood, we can use the original height again)
						Wc=Zc;
						continueIncrementation=true;
						//nothing else to be done in the neighbor loop
						break;
					}
					//Operation 2 applicable?
					else if (Wc > Wn + e)
					{
						//Do Operation 2 (If there is a lower new height than the actual cell height in the neighborhood, we use that height)
						Wc = Wn + e;
						continueIncrementation=true;
						++NoOfActions;
					}
				} //for Neighbors
			}
		} // For cells in oldHeightMap
	} //As long there's something to do
	delete &oldHeightMap;
	return NoOfActions;

}