#include "flux_node.h"
#include "flux_connection.h"
#include "collections.h"
#include "../project.h"

static int nextnodeid=0;
// Registers a connection at this node (only called by ctor of flux_connection)

void cmf::water::flux_node::RegisterConnection(flux_connection* newConnection )
{
	// Get the other end of the connection
	int key=(newConnection->get_target(*this))->node_id;
	// Register the connection
	if (newConnection->weak_this.expired())
	{
		m_Connections[key] = flux_connection::ptr(newConnection);
		newConnection->weak_this = m_Connections[key];
	}
	else
		m_Connections[key] = newConnection->weak_this.lock();

}
// Deregisters  a connection at this node (only called by dtor of flux_connection)

void cmf::water::flux_node::DeregisterConnection(flux_connection* oldConnection)
{
	// throw std::runtime_error("Do we really need DeregisterConnection");
	// loops through the existing connections for deregister it. 
	// The fast access m_Connections.erase(oldconnection->get_target(this)->node_id) does not work, since
	// the target of old connection might be expired already
	for(ConnectionMap::iterator it = m_Connections.begin(); it != m_Connections.end(); ++it)
	{
		if (it->second.get() == oldConnection)
		{
			m_Connections.erase(it->first);
			return;
		}
	}
}


cmf::water::flux_connection* cmf::water::flux_node::get_connection( const cmf::water::flux_node& target )
{
	if (m_Connections.find(target.node_id)!=m_Connections.end())
		return m_Connections[target.node_id].get();
	else
		return 0;
}


real cmf::water::flux_node::flux_to( const flux_node& target,cmf::math::Time t )
{
	if (m_Connections.find(target.node_id)!=m_Connections.end())
		return -m_Connections[target.node_id]->q(*this,t);
	else
		return 0;
}



real cmf::water::flux_node::water_balance( cmf::math::Time t,const flux_connection* except/*=0*/ )
{
	real waterbalance=0;
	for(flux_node::ConnectionMap::iterator it = m_Connections.begin(); it != m_Connections.end(); ++it)
	{
		flux_connection* conn=it->second.get();
		if (conn!=except)
			waterbalance+=conn->q(*this,t);
	}
	return waterbalance;
}


// dtor of flux_node, deletes all connections
cmf::water::flux_node::~flux_node()
{
	while (m_Connections.size())
	{
		flux_connection::ptr con = m_Connections.begin()->second;
		m_Connections.erase(m_Connections.begin());
		con->kill_me();
	}
	if (project().debug)
		std::cout << "Deleted " << Name << std::endl;
}



bool cmf::water::flux_node::remove_connection( flux_node::ptr To )
{
	if (To)
	{
		flux_connection* con = get_connection(*To);
		if (con) return con->kill_me();
	}
	return false;
}

real cmf::water::flux_node::conc( cmf::math::Time t, const cmf::water::solute& solute ) const
{
	real influx_sum_water=0,influx_sum_solute=0;
	for(flux_node::ConnectionMap::const_iterator it = m_Connections.begin(); it != m_Connections.end(); ++it)
	{
		real q=it->second->q(*this,t);
		if (q>0)
		{
			influx_sum_water  += q;
			influx_sum_solute += q * it->second->conc(t,solute);
		}
	}
	return influx_sum_water >= 0 ? influx_sum_solute/influx_sum_water : 0.0;
}


cmf::water::flux_node::flux_node( const cmf::project& _project,cmf::geometry::point location ) 
: m_project(_project), node_id(nextnodeid++),Location(location)
{
}


cmf::geometry::point cmf::water::flux_node::flux3d_to( const cmf::water::flux_node& target,cmf::math::Time t )
{
	if (m_Connections.find(target.node_id)!=m_Connections.end())
		return -m_Connections[target.node_id]->q(*this,t) * get_direction_to(target);
	else
		return cmf::geometry::point();
}


cmf::geometry::point cmf::water::flux_node::get_3d_flux( cmf::math::Time t )
{
	cmf::geometry::point res;
	for(flux_node::ConnectionMap::iterator it = m_Connections.begin(); it != m_Connections.end(); ++it)
	{
		flux_node::ptr target=it->second->get_target(*this);
		real f=flux_to(*target,t);
		cmf::geometry::point dir=get_direction_to(*target);
		res+= dir * f;
	}
	return res;


}

cmf::water::connection_vector cmf::water::flux_node::get_connections() const
{
	connection_vector res;
	for(ConnectionMap::const_iterator it = m_Connections.begin(); it != m_Connections.end(); ++it)
	{		
		res.push_back(it->second.get());
	}
	return res;
}
int cmf::water::count_node_references( flux_node::ptr node )
{
	return int(node.use_count());
}