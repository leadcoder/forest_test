#pragma once
#include "Common.h"
#include <osg/StateSet>
#include <osg/Geometry>
#include <math.h>
#include "IMeshRenderingTech.h"

namespace osgVegetation
{
	class osgvExport MRTShaderInstancing :  public IMeshRenderingTech
	{
	public:
		MRTShaderInstancing() {}
		osg::StateSet* createStateSet(MeshLayerVector &layers);
		//osg::Node* create(const MeshVegetationObjectVector &trees);
		osg::Node* create(const MeshVegetationObjectVector &trees, const std::string &mesh_name, const osg::BoundingBox &bb);
		osg::StateSet* m_StateSet ; 
	protected:
		std::map<std::string, osg::ref_ptr<osg::Node>  > m_MeshNodeMap;
		std::vector<osg::Geometry*> m_Geometries;
	};
}