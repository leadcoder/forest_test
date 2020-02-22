#pragma once

#include <osg/Geometry>
#include <osg/Depth>
#include <osg/Point>
#include <osg/VertexAttribDivisor>
#include <osgDB/ReadFile>
#include <osgDB/FileUtils>
#include <osgUtil/Optimizer>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Multisample>

class XBFInstance : public osg::Group
{
public:
	/**
	* Draw callback that goes on the instanced geometry, and draws exactly the number of
	* instances required (instead of reading the information from PrimitiveSet::getNumInstances)
	*/
	struct InstanceDrawCallback : public osg::Drawable::DrawCallback
	{
		InstanceDrawCallback() : _numInstances(0) { }

		void drawImplementation(osg::RenderInfo& renderInfo, const osg::Drawable* drawable) const
		{
			if (_numInstances == 0)
				return;

			const osg::Geometry* geom = static_cast<const osg::Geometry*>(drawable);
			geom->drawVertexArraysImplementation(renderInfo);
			for (unsigned int i = 0; i < geom->getNumPrimitiveSets(); ++i)
			{
				const osg::PrimitiveSet* pset = geom->getPrimitiveSet(i);
				const osg::DrawArrays* da = dynamic_cast<const osg::DrawArrays*>(pset);
				if (da)
				{
					renderInfo.getState()->glDrawArraysInstanced(da->getMode(), da->getFirst(), da->getCount(), _numInstances);
				}
				else
				{
					const osg::DrawElements* de = dynamic_cast<const osg::DrawElements*>(pset);
					if (de)
					{
						GLenum dataType = const_cast<osg::DrawElements*>(de)->getDataType();
						renderInfo.getState()->glDrawElementsInstanced(de->getMode(), de->getNumIndices(), dataType, de->getDataPointer(), _numInstances);
					}
				}
			}
		}
		unsigned int _numInstances;
	};

	class ConvertToInstancing : public osg::NodeVisitor
	{
	public:
		ConvertToInstancing(XBFInstance* ig) : _ig(ig), osg::NodeVisitor()
		{
			setTraversalMode(TRAVERSE_ALL_CHILDREN);
			this->setNodeMaskOverride(~0);
		}

		void apply(osg::Geode& geode)
		{
			for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
			{
				osg::Geometry* geom = geode.getDrawable(i)->asGeometry();
				if (geom)
				{
					// Must ensure that VBOs are used:
					geom->setUseVertexBufferObjects(true);
					geom->setUseDisplayList(false);

					// Bind our transform feedback buffer to the geometry:
					geom->setVertexAttribArray(_ig->_positionSlot, _ig->_xfbPosition.get());
					geom->setVertexAttribBinding(_ig->_positionSlot, geom->BIND_PER_VERTEX);
					geom->setVertexAttribNormalize(_ig->_positionSlot, false);
				
					geom->setInitialBound(_ig->_bounds);

					// Set up a draw callback to intecept draw calls so we can vary 
					// the instance count per frame.
					geom->setDrawCallback(_ig->_drawCallback.get());
					// disable frustum culling because the instance doesn't have a real location
					geom->setCullingActive(false);
				}
			}
		}
	private:
		XBFInstance* _ig;
	};

	XBFInstance(unsigned int maxNumInstances,
		int slot, 
		osg::Node* model, 
		const osg::BoundingBox &bbox) : 
		_maxNumInstances(maxNumInstances),
		_positionSlot(slot),
		_bounds(bbox),
		_xfbPosition(new osg::Vec4Array()),
		_drawCallback(new InstanceDrawCallback())
	{
		_xfbPosition->resizeArray(maxNumInstances);
		osg::StateSet* model_ss = model->getOrCreateStateSet();
		osg::Program* prog = dynamic_cast<osg::Program*>(model_ss->getAttribute(osg::StateAttribute::PROGRAM));
		osg::StateSet* ss = getOrCreateStateSet();
		if (prog == NULL)
		{
			_shaderProgram = makeRenderProgram(prog == NULL);
			ss->setAttribute(_shaderProgram, osg::StateAttribute::OVERRIDE);
			osg::Uniform* baseTextureSampler = new osg::Uniform("ov_color_texture", 0);
			ss->addUniform(baseTextureSampler);
		}
		else
		{
			//_shaderProgram = prog;
			_shaderProgram = makeRenderProgram(true);
			ss->setAttribute(_shaderProgram, osg::StateAttribute::OVERRIDE);
			osg::StateSet::DefineList& defineList = ss->getDefineList();
			defineList["OV_BILLBOARD"].second = (osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
		}
			
		
		_shaderProgram->removeBindAttribLocation("xfb_position");
		_shaderProgram->addBindAttribLocation("xfb_position", _positionSlot);
		ss->setAttribute(new osg::VertexAttribDivisor(_positionSlot, 1));
		
		//osg::AlphaFunc* alphaFunc = new osg::AlphaFunc;
		//alphaFunc->setFunction(osg::AlphaFunc::GEQUAL, 0.5);
		//ss->setAttributeAndModes(alphaFunc, osg::StateAttribute::OVERRIDE);
		//ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

		if (false)
		{
			ss->setAttributeAndModes(new osg::BlendFunc, osg::StateAttribute::ON);
		}
		else
		{
			ss->setMode(GL_SAMPLE_ALPHA_TO_COVERAGE_ARB, 1);
			ss->setAttributeAndModes(new osg::BlendFunc(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO), osg::StateAttribute::OVERRIDE);
		}
		
		// In practice, we will pre-set a bounding sphere/box for a tile
		this->setCullingActive(false);
	
		addChild(model);

		//convert model to use instancing
		ConvertToInstancing visitor(this);
		this->accept(visitor);
	}

	void setNumInstancesToDraw(unsigned int num)
	{
		_drawCallback->_numInstances = num;
	}

	osg::Array* getPositions() const
	{
		return _xfbPosition.get();
	}

private:
	osg::Program* makeRenderProgram(bool add_fragment_program)
	{
		osg::Program* program = new osg::Program;
		program->addShader(osg::Shader::readShaderFile(osg::Shader::VERTEX, osgDB::findDataFile("xbf_render_vertex.glsl")));
		if(add_fragment_program)
			program->addShader(osg::Shader::readShaderFile(osg::Shader::FRAGMENT, osgDB::findDataFile("xbf_render_fragment.glsl")));
		return program;
	}

	osg::BoundingSphere computeBound() const
	{
		osg::BoundingSphere bs(_bounds);
		return bs;
	}

	unsigned int _maxNumInstances;
	int _positionSlot;
	osg::ref_ptr<osg::Array> _xfbPosition;
	osg::ref_ptr<InstanceDrawCallback> _drawCallback;
	osg::BoundingSphere _xbfBounds;
	osg::BoundingBox _bounds;
	osg::Program *_shaderProgram;
};