#pragma once
#include "Common.h"
#include <osg/Referenced>
#include <osg/Vec4>
#include <osg/Vec3>
#include <osg/Vec2>
#include "CoverageColor.h"
namespace osgVegetation
{
	/**
		Interface for terrain queries
	*/
	class osgvExport ITerrainQuery : public osg::Referenced
	{
	public:
		virtual ~ITerrainQuery(){};
		/**
			Get terrain data for provided location
		*/
		virtual bool getTerrainData(osg::Vec3& location, osg::Vec4 &color, std::string &coverage_name , CoverageColor &coverage_color, osg::Vec3 &inter) = 0;
	};
}
