/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2014 Robert Osfield
 *  Copyright (C) 2014 Pawel Ksiezopolski
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
 *
*/

#ifndef AGREGATE_GEOMETRY_VISITOR_H
#define AGREGATE_GEOMETRY_VISITOR_H 1
#include "ov_Register.h"
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Transform>
#include <osg/NodeVisitor>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2DArray>
#include <osg/Texture2D>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Multisample>

// AggregateGeometryVisitor uses ConvertTrianglesOperator to create and fill osg::Geometry
// with data matching users needs
struct ConvertTrianglesOperator : public osg::Referenced
{
    ConvertTrianglesOperator()
        : osg::Referenced()
    {
    }
    virtual void initGeometry( osg::Geometry* outputGeometry ) = 0;
    virtual bool pushNode( osg::Node* /*node*/ )
    {
        return false;
    }
    virtual void popNode() = 0;
    virtual void setGeometryData( int tex_index, const osg::Matrix &matrix, osg::Geometry *inputGeometry, osg::Geometry* outputGeometry, float typeID, float lodNumber ) = 0;
    virtual void operator() ( unsigned int i1, unsigned int i2, unsigned int i3 ) = 0;
};

class GetVec2FromArrayVisitor : public osg::ValueVisitor
{
public:
    GetVec2FromArrayVisitor()
    {
    }
    void apply( GLfloat& value )
    {
        out = osg::Vec2( value, 0.0 );
    }
    void apply( osg::Vec2& value )
    {
        out = osg::Vec2( value.x(), value.y() );
    }
    virtual void apply( osg::Vec2d& value )
    {
        out = osg::Vec2( value.x(), value.y() );
    }
    void apply( osg::Vec3& value )
    {
        out = osg::Vec2( value.x(), value.y() );
    }
    void apply( osg::Vec4& value )
    {
        out = osg::Vec2( value.x(), value.y() );
    }
    void apply( osg::Vec3d& value )
    {
        out = osg::Vec2( value.x(), value.y() );
    }
    void apply( osg::Vec4d& value )
    {
        out = osg::Vec2( value.x(), value.y() );
    }

    osg::Vec2 out;
};


// ConvertTrianglesOperatorClassic is a ConvertTrianglesOperator that creates
// aggregated geometry with standard set of vertex attributes : vertices, normals, color and texCoord0.
// texCoord1 holds additional information about vertex ( typeID, lodNumber, boneIndex )
struct ConvertTrianglesOperatorClassic : public ConvertTrianglesOperator
{
    ConvertTrianglesOperatorClassic()
        : ConvertTrianglesOperator(), _typeID(0.0f), _lodNumber(0.0f)

    {
        _boneIndices.push_back(0.0);
    }
    virtual void initGeometry( osg::Geometry* outputGeometry )
    {
        osg::Vec3Array* vertices    = new osg::Vec3Array; outputGeometry->setVertexArray( vertices );
        osg::Vec4Array* colors      = new osg::Vec4Array; outputGeometry->setColorArray( colors, osg::Array::BIND_PER_VERTEX );
        osg::Vec3Array* normals     = new osg::Vec3Array; outputGeometry->setNormalArray( normals, osg::Array::BIND_PER_VERTEX );
        osg::Vec2Array* texCoords0  = new osg::Vec2Array; outputGeometry->setTexCoordArray( 0, texCoords0 );
        osg::Vec3Array* texCoords1  = new osg::Vec3Array; outputGeometry->setTexCoordArray( 1, texCoords1 );
        outputGeometry->setStateSet(NULL);
    }
    virtual bool pushNode( osg::Node* node )
    {
        std::map<std::string,float>::iterator it = _boneNames.find( node->getName() );
        if(it==_boneNames.end())
            return false;
        _boneIndices.push_back( it->second );
        return true;
    }
    virtual void popNode()
    {
        _boneIndices.pop_back();
    }
    virtual void setGeometryData(int tex_index,  const osg::Matrix &matrix, osg::Geometry *inputGeometry, osg::Geometry* outputGeometry, float typeID, float lodNumber )
    {
        _matrix = matrix;
		_texIndex = tex_index;
        _inputVertices  = dynamic_cast<osg::Vec3Array *>( inputGeometry->getVertexArray() );
        _inputColors    = dynamic_cast<osg::Vec4Array *>( inputGeometry->getColorArray() );
        _inputNormals   = dynamic_cast<osg::Vec3Array *>( inputGeometry->getNormalArray() );
        _inputTexCoord0 = inputGeometry->getTexCoordArray(0);

        _outputVertices = dynamic_cast<osg::Vec3Array *>( outputGeometry->getVertexArray() );
        _outputColors   = dynamic_cast<osg::Vec4Array *>( outputGeometry->getColorArray() );
        _outputNormals  = dynamic_cast<osg::Vec3Array *>( outputGeometry->getNormalArray() );
        _outputTexCoord0 = dynamic_cast<osg::Vec2Array *>( outputGeometry->getTexCoordArray(0) );
        _outputTexCoord1 = dynamic_cast<osg::Vec3Array *>( outputGeometry->getTexCoordArray(1) );

        _typeID = typeID;
        _lodNumber = lodNumber;
    }
    virtual void operator() ( unsigned int i1, unsigned int i2, unsigned int i3 )
    {
        unsigned int ic1=i1, ic2=i2, ic3=i3, in1=i1, in2=i2, in3=i3, it01=i1, it02=i2, it03=i3;
        if ( _inputColors!=NULL && _inputColors->size() == 1 )
        {
            ic1=0; ic2=0; ic3=0;
        }
        if ( _inputNormals!=NULL && _inputNormals->size() == 1 )
        {
            in1=0; in2=0; in3=0;
        }
        if ( _inputTexCoord0!=NULL && _inputTexCoord0->getNumElements()==1 )
        {
            it01=0; it02=0; it03=0;
        }

        _outputVertices->push_back( _inputVertices->at( i1 ) * _matrix );
        _outputVertices->push_back( _inputVertices->at( i2 ) * _matrix  );
        _outputVertices->push_back( _inputVertices->at( i3 ) * _matrix  );

        if( _inputColors != NULL )
        {
            _outputColors->push_back( _inputColors->at( ic1 ) );
            _outputColors->push_back( _inputColors->at( ic2 ) );
            _outputColors->push_back( _inputColors->at( ic3 ) );
        }
        else
        {
            for(unsigned int i=0; i<3; ++i)
                _outputColors->push_back( osg::Vec4(1.0,1.0,1.0,1.0) );
        }

        if( _inputNormals != NULL )
        {
            _outputNormals->push_back( osg::Matrix::transform3x3( _inputNormals->at( in1 ), _matrix ) );
            _outputNormals->push_back( osg::Matrix::transform3x3( _inputNormals->at( in2 ), _matrix ) );
            _outputNormals->push_back( osg::Matrix::transform3x3( _inputNormals->at( in3 ), _matrix ) );
        }
        else
        {
            for(unsigned int i=0; i<3; ++i)
                _outputNormals->push_back( osg::Vec3( 0.0,0.0,1.0 ) );
        }
        if( _inputTexCoord0 != NULL )
        {
            _inputTexCoord0->accept( it01, _inputTexCoord0Visitor );
            _outputTexCoord0->push_back( _inputTexCoord0Visitor.out );
            _inputTexCoord0->accept( it02, _inputTexCoord0Visitor );
            _outputTexCoord0->push_back( _inputTexCoord0Visitor.out );
            _inputTexCoord0->accept( it03, _inputTexCoord0Visitor );
            _outputTexCoord0->push_back( _inputTexCoord0Visitor.out );
        }
        else
        {
            for(unsigned int i=0; i<3; ++i)
                _outputTexCoord0->push_back( osg::Vec2(0.0,0.0) );
        }

        for(unsigned int i=0; i<3; ++i)
			_outputTexCoord1->push_back(osg::Vec3(_typeID, _lodNumber, _texIndex));
            //_outputTexCoord1->push_back( osg::Vec3( _typeID, _lodNumber, _boneIndices.back() ) );
    }
    void registerBoneByName( const std::string& boneName, int boneIndex )
    {
        _boneNames[boneName] = float(boneIndex);
    }

    osg::Matrix _matrix;

    osg::Vec3Array* _inputVertices;
    osg::Vec4Array* _inputColors;
    osg::Vec3Array* _inputNormals;
    osg::Array*     _inputTexCoord0;

    osg::Vec3Array* _outputVertices;
    osg::Vec4Array* _outputColors;
    osg::Vec3Array* _outputNormals;
    osg::Vec2Array* _outputTexCoord0;
    osg::Vec3Array* _outputTexCoord1;

    float _typeID;
    float _lodNumber;
	float _texIndex;
    std::vector<float> _boneIndices;

    std::map<std::string,float> _boneNames;

    GetVec2FromArrayVisitor _inputTexCoord0Visitor;
};



class AggregateGeometryVisitor : public osg::NodeVisitor
{
public:
    AggregateGeometryVisitor(ConvertTrianglesOperator* ctOperator)
		: osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
	{
		_ctOperator.setConverter(ctOperator);
		init();
	}

	void init()
	{
		_aggregatedGeometry = new osg::Geometry;
		_ctOperator.initGeometry(_aggregatedGeometry.get());
		_matrixStack.clear();
	}

    // osg::TriangleIndexFunctor uses its template parameter as a base class, so we must use an adapter pattern to hack it
    struct ConvertTrianglesBridge
    {
        inline void setConverter( ConvertTrianglesOperator* cto )
        {
            _converter = cto;
        }
        inline void initGeometry( osg::Geometry* outputGeometry )
        {
            _converter->initGeometry(outputGeometry);
        }
        inline bool pushNode( osg::Node* node )
        {
            return _converter->pushNode( node );
        }
        inline void popNode()
        {
            _converter->popNode();
        }
        inline void setGeometryData(int tex_index, const osg::Matrix &matrix, osg::Geometry *inputGeometry, osg::Geometry* outputGeometry, float typeID, float lodNumber )
        {
            _converter->setGeometryData(tex_index,matrix,inputGeometry,outputGeometry,typeID, lodNumber);
        }
        inline void operator() ( unsigned int i1, unsigned int i2, unsigned int i3 )
        {
            _converter->operator()(i1,i2,i3);
        }

        osg::ref_ptr<ConvertTrianglesOperator> _converter;
    };


    // struct returning information about added object ( first vertex, vertex count, primitiveset index )
    // used later to create indirect command texture buffers
    struct AddObjectResult
    {
        AddObjectResult( unsigned int f, unsigned int c, unsigned int i )
            : first(f), count(c), index(i)
        {
        }
        unsigned int first;
        unsigned int count;
        unsigned int index;
    };

	AddObjectResult addObject(osg::Node* object, unsigned int typeID, unsigned int lodNumber)
	{
		unsigned int currentVertexFirst = _aggregatedGeometry->getVertexArray()->getNumElements();
		_currentTypeID = typeID;
		_currentLodNumber = lodNumber;
		object->accept(*this);
		unsigned int currentVertexCount = _aggregatedGeometry->getVertexArray()->getNumElements() - currentVertexFirst;
		_aggregatedGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES, currentVertexFirst, currentVertexCount));
		_matrixStack.clear();
		return AddObjectResult(currentVertexFirst, currentVertexCount, _aggregatedGeometry->getNumPrimitiveSets() - 1);
	}

	void apply(osg::Node& node)
	{
		bool pushed = _ctOperator.pushNode(&node);
		traverse(node);
		if (pushed) _ctOperator.popNode();

		
	}

	void apply(osg::Transform& transform)
	{
		bool pushed = _ctOperator.pushNode(&transform);
		osg::Matrix matrix;
		if (!_matrixStack.empty())
			matrix = _matrixStack.back();
		transform.computeLocalToWorldMatrix(matrix, this);
		_matrixStack.push_back(matrix);

		traverse(transform);

		_matrixStack.pop_back();
		if (pushed) _ctOperator.popNode();
	}

	void apply(osg::Geode& geode)
	{
		bool pushed = _ctOperator.pushNode(&geode);

		osg::Matrix matrix;
		if (!_matrixStack.empty())
			matrix = _matrixStack.back();

		int tex_index = 0;
		osg::Texture2D* tex = dynamic_cast<osg::Texture2D*>(geode.getOrCreateStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
		if (tex)
		{
			tex_index = getOrAddImage(tex->getImage());
		}
		else
		{
			tex = dynamic_cast<osg::Texture2D*>(geode.getParent(0)->getOrCreateStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
			if (tex)
			{
				tex_index = getOrAddImage(tex->getImage());
			}
		}

		for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
		{
			osg::Geometry* geom = geode.getDrawable(i)->asGeometry();

			if (geom != NULL)
			{
				if (tex == NULL)
				{
					osg::Texture2D* geom_tex = dynamic_cast<osg::Texture2D*>(geom->getOrCreateStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
					if (geom_tex)
					{
						tex_index = getOrAddImage(geom_tex->getImage());
					}
				}
				_ctOperator.setGeometryData(tex_index, matrix, geom, _aggregatedGeometry.get(), (float)_currentTypeID, (float)_currentLodNumber);
				geom->accept(_ctOperator);
			}
		}

		traverse(geode);
		if (pushed) _ctOperator.popNode();
	}

    /*inline osg::Geometry* getAggregatedGeometry()
    {
        return _aggregatedGeometry.get();
    }*/

	inline osg::ref_ptr <osg::Geometry> getAggregatedGeometry()
	{
		return _aggregatedGeometry;
	}

	int getOrAddImage(osg::Image* image)
	{
		if (image->getFileName() != "")
		{
			for (size_t i = 0; i < _images.size(); i++)
			{
				if (image->getFileName() == _images[i]->getFileName())
					return i;
			}
		}

		osg::Image* new_image = dynamic_cast<osg::Image*>(image->clone(osg::CopyOp::DEEP_COPY_STATESETS));
		if (new_image)
		{
			_images.push_back(new_image);
		}
		return _images.size()-1;
	}

	osg::ref_ptr<osg::Texture2DArray> generateTextureArray()
	{
		int tex_size = 1024;
		osg::ref_ptr<osg::Texture2DArray> tex = new osg::Texture2DArray;
		tex->setTextureSize(tex_size, tex_size, _images.size());
		tex->setUseHardwareMipMapGeneration(true);
		tex->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
		tex->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
		//tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
		for (size_t i = 0; i < _images.size(); i++)
		{
			_images[i]->scaleImage(tex_size, tex_size, 1);
			tex->setImage(i, _images[i]);
		}
		const int mesh_tex_unit = osgVegetation::Register.TexUnits.CreateOrGetUnit(OV_MESH_COLOR_TEXTURE_ID);
		_aggregatedGeometry->getOrCreateStateSet()->setTextureAttributeAndModes(mesh_tex_unit, tex, osg::StateAttribute::ON);
		osg::Uniform* baseTextureSampler = new osg::Uniform("ov_mesh_color_texture", mesh_tex_unit);
		_aggregatedGeometry->getOrCreateStateSet()->addUniform(baseTextureSampler);

		_aggregatedGeometry->getOrCreateStateSet()->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB, 1);
		//_aggregatedGeometry->getOrCreateStateSet()->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO), osg::StateAttribute::OVERRIDE);

		//need this for shadows to work
		osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
		alphaFunc->setFunction(osg::AlphaFunc::GEQUAL, 0.1);
		_aggregatedGeometry->getOrCreateStateSet()->setAttributeAndModes(alphaFunc, osg::StateAttribute::ON);

		return tex;
	}
protected:
    osg::ref_ptr<osg::Geometry> _aggregatedGeometry;
    osg::TriangleIndexFunctor<ConvertTrianglesBridge> _ctOperator;
    std::vector<osg::Matrix>    _matrixStack;
	std::vector < osg::Image* >	 _images;
    unsigned int _currentTypeID;
    unsigned int _currentLodNumber;
};

#endif
