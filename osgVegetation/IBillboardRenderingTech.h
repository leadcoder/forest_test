#pragma once
#include "Common.h"
#include "BillboardLayer.h"
#include "BillboardObject.h"

#include <osg/StateSet>
#include <osg/Geometry>
#include <osg/Node>
#include <math.h>


namespace osgVegetation
{
	class IBillboardRenderingTech : public osg::Referenced
	{
	public:
		IBillboardRenderingTech(){}
		virtual ~IBillboardRenderingTech(){}
		virtual osg::Node* create(double view_dist, const BillboardVegetationObjectVector &trees, const osg::BoundingBox &bb) = 0;
		virtual osg::StateSet* getStateSet() const = 0;
	protected:
	};
}
