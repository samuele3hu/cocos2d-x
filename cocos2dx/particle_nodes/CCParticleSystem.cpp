/****************************************************************************
Copyright (c) 2010-2012 cocos2d-x.org
Copyright (c) 2008-2010 Ricardo Quesada
Copyright (c) 2011      Zynga Inc.

http://www.cocos2d-x.org

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
****************************************************************************/

// ideas taken from:
//     . The ocean spray in your face [Jeff Lander]
//        http://www.double.co.nz/dust/col0798.pdf
//     . Building an Advanced Particle System [John van der Burg]
//        http://www.gamasutra.com/features/20000623/vanderburg_01.htm
//   . LOVE game engine
//        http://love2d.org/
//
//
// Radius mode support, from 71 squared
//        http://particledesigner.71squared.com/
//
// IMPORTANT: Particle Designer is supported by cocos2d, but
// 'Radius Mode' in Particle Designer uses a fixed emit rate of 30 hz. Since that can't be guaranteed in cocos2d,
//  cocos2d uses a another approach, but the results are almost identical. 
//

#include "CCParticleSystem.h"

#include <string>

#include "CCParticleBatchNode.h"
#include "ccTypes.h"
#include "textures/CCTextureCache.h"
#include "textures/CCTextureAtlas.h"
#include "support/base64.h"
#include "platform/CCFileUtils.h"
#include "platform/CCImage.h"
#include "support/zip_support/ZipUtils.h"
#include "CCDirector.h"
#include "support/CCProfiling.h"
// opengl
#include "CCGL.h"

using namespace std;


NS_CC_BEGIN

// ideas taken from:
//     . The ocean spray in your face [Jeff Lander]
//        http://www.double.co.nz/dust/col0798.pdf
//     . Building an Advanced Particle System [John van der Burg]
//        http://www.gamasutra.com/features/20000623/vanderburg_01.htm
//   . LOVE game engine
//        http://love2d.org/
//
//
// Radius mode support, from 71 squared
//        http://particledesigner.71squared.com/
//
// IMPORTANT: Particle Designer is supported by cocos2d, but
// 'Radius Mode' in Particle Designer uses a fixed emit rate of 30 hz. Since that can't be guaranteed in cocos2d,
//  cocos2d uses a another approach, but the results are almost identical. 
//

ParticleSystem::ParticleSystem()
: _plistFile("")
, _elapsed(0)
, _particles(NULL)
, _emitCounter(0)
, _particleIdx(0)
, _batchNode(NULL)
, _atlasIndex(0)
, _transformSystemDirty(false)
, _allocatedParticles(0)
, _isActive(true)
, _particleCount(0)
, _duration(0)
, _sourcePosition(Point::ZERO)
, _posVar(Point::ZERO)
, _life(0)
, _lifeVar(0)
, _angle(0)
, _angleVar(0)
, _startSize(0)
, _startSizeVar(0)
, _endSize(0)
, _endSizeVar(0)
, _startSpin(0)
, _startSpinVar(0)
, _endSpin(0)
, _endSpinVar(0)
, _emissionRate(0)
, _totalParticles(0)
, _texture(NULL)
, _opacityModifyRGB(false)
, _isBlendAdditive(false)
, _positionType(POSITION_TYPE_FREE)
, _isAutoRemoveOnFinish(false)
, _emitterMode(MODE_GRAVITY)
{
    modeA.gravity = Point::ZERO;
    modeA.speed = 0;
    modeA.speedVar = 0;
    modeA.tangentialAccel = 0;
    modeA.tangentialAccelVar = 0;
    modeA.radialAccel = 0;
    modeA.radialAccelVar = 0;
    modeA.rotationIsDir = false;
    modeB.startRadius = 0;
    modeB.startRadiusVar = 0;
    modeB.endRadius = 0;
    modeB.endRadiusVar = 0;            
    modeB.rotatePerSecond = 0;
    modeB.rotatePerSecondVar = 0;
    _blendFunc.src = CC_BLEND_SRC;
    _blendFunc.dst = CC_BLEND_DST;
}
// implementation ParticleSystem

ParticleSystem * ParticleSystem::create(const char *plistFile)
{
    ParticleSystem *pRet = new ParticleSystem();
    if (pRet && pRet->initWithFile(plistFile))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return pRet;
}

ParticleSystem* ParticleSystem::createWithTotalParticles(unsigned int numberOfParticles)
{
    ParticleSystem *pRet = new ParticleSystem();
    if (pRet && pRet->initWithTotalParticles(numberOfParticles))
    {
        pRet->autorelease();
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return pRet;
}

bool ParticleSystem::init()
{
    return initWithTotalParticles(150);
}

bool ParticleSystem::initWithFile(const char *plistFile)
{
    bool bRet = false;
    _plistFile = FileUtils::getInstance()->fullPathForFilename(plistFile);
    Dictionary *dict = Dictionary::createWithContentsOfFileThreadSafe(_plistFile.c_str());

    CCASSERT( dict != NULL, "Particles: file not found");
    
    // XXX compute path from a path, should define a function somewhere to do it
    string listFilePath = plistFile;
    if (listFilePath.find('/') != string::npos)
    {
        listFilePath = listFilePath.substr(0, listFilePath.rfind('/') + 1);
        bRet = this->initWithDictionary(dict, listFilePath.c_str());
    }
    else
    {
        bRet = this->initWithDictionary(dict, "");
    }
    
    dict->release();

    return bRet;
}

bool ParticleSystem::initWithDictionary(Dictionary *dictionary)
{
    return initWithDictionary(dictionary, "");
}

bool ParticleSystem::initWithDictionary(Dictionary *dictionary, const char *dirname)
{
    bool bRet = false;
    unsigned char *buffer = NULL;
    unsigned char *deflated = NULL;
    Image *image = NULL;
    do 
    {
        int maxParticles = dictionary->valueForKey("maxParticles")->intValue();
        // self, not super
        if(this->initWithTotalParticles(maxParticles))
        {
            // angle
            _angle = dictionary->valueForKey("angle")->floatValue();
            _angleVar = dictionary->valueForKey("angleVariance")->floatValue();

            // duration
            _duration = dictionary->valueForKey("duration")->floatValue();

            // blend function 
            _blendFunc.src = dictionary->valueForKey("blendFuncSource")->intValue();
            _blendFunc.dst = dictionary->valueForKey("blendFuncDestination")->intValue();

            // color
            _startColor.r = dictionary->valueForKey("startColorRed")->floatValue();
            _startColor.g = dictionary->valueForKey("startColorGreen")->floatValue();
            _startColor.b = dictionary->valueForKey("startColorBlue")->floatValue();
            _startColor.a = dictionary->valueForKey("startColorAlpha")->floatValue();

            _startColorVar.r = dictionary->valueForKey("startColorVarianceRed")->floatValue();
            _startColorVar.g = dictionary->valueForKey("startColorVarianceGreen")->floatValue();
            _startColorVar.b = dictionary->valueForKey("startColorVarianceBlue")->floatValue();
            _startColorVar.a = dictionary->valueForKey("startColorVarianceAlpha")->floatValue();

            _endColor.r = dictionary->valueForKey("finishColorRed")->floatValue();
            _endColor.g = dictionary->valueForKey("finishColorGreen")->floatValue();
            _endColor.b = dictionary->valueForKey("finishColorBlue")->floatValue();
            _endColor.a = dictionary->valueForKey("finishColorAlpha")->floatValue();

            _endColorVar.r = dictionary->valueForKey("finishColorVarianceRed")->floatValue();
            _endColorVar.g = dictionary->valueForKey("finishColorVarianceGreen")->floatValue();
            _endColorVar.b = dictionary->valueForKey("finishColorVarianceBlue")->floatValue();
            _endColorVar.a = dictionary->valueForKey("finishColorVarianceAlpha")->floatValue();

            // particle size
            _startSize = dictionary->valueForKey("startParticleSize")->floatValue();
            _startSizeVar = dictionary->valueForKey("startParticleSizeVariance")->floatValue();
            _endSize = dictionary->valueForKey("finishParticleSize")->floatValue();
            _endSizeVar = dictionary->valueForKey("finishParticleSizeVariance")->floatValue();

            // position
            float x = dictionary->valueForKey("sourcePositionx")->floatValue();
            float y = dictionary->valueForKey("sourcePositiony")->floatValue();
            this->setPosition( Point(x,y) );            
            _posVar.x = dictionary->valueForKey("sourcePositionVariancex")->floatValue();
            _posVar.y = dictionary->valueForKey("sourcePositionVariancey")->floatValue();

            // Spinning
            _startSpin = dictionary->valueForKey("rotationStart")->floatValue();
            _startSpinVar = dictionary->valueForKey("rotationStartVariance")->floatValue();
            _endSpin= dictionary->valueForKey("rotationEnd")->floatValue();
            _endSpinVar= dictionary->valueForKey("rotationEndVariance")->floatValue();

            _emitterMode = dictionary->valueForKey("emitterType")->intValue();

            // Mode A: Gravity + tangential accel + radial accel
            if (_emitterMode == MODE_GRAVITY)
            {
                // gravity
                modeA.gravity.x = dictionary->valueForKey("gravityx")->floatValue();
                modeA.gravity.y = dictionary->valueForKey("gravityy")->floatValue();

                // speed
                modeA.speed = dictionary->valueForKey("speed")->floatValue();
                modeA.speedVar = dictionary->valueForKey("speedVariance")->floatValue();

                // radial acceleration
                modeA.radialAccel = dictionary->valueForKey("radialAcceleration")->floatValue();
                modeA.radialAccelVar = dictionary->valueForKey("radialAccelVariance")->floatValue();

                // tangential acceleration
                modeA.tangentialAccel = dictionary->valueForKey("tangentialAcceleration")->floatValue();
                modeA.tangentialAccelVar = dictionary->valueForKey("tangentialAccelVariance")->floatValue();
                
                // rotation is dir
                modeA.rotationIsDir = dictionary->valueForKey("rotationIsDir")->boolValue();
            }

            // or Mode B: radius movement
            else if (_emitterMode == MODE_RADIUS)
            {
                modeB.startRadius = dictionary->valueForKey("maxRadius")->floatValue();
                modeB.startRadiusVar = dictionary->valueForKey("maxRadiusVariance")->floatValue();
                modeB.endRadius = dictionary->valueForKey("minRadius")->floatValue();
                modeB.endRadiusVar = 0.0f;
                modeB.rotatePerSecond = dictionary->valueForKey("rotatePerSecond")->floatValue();
                modeB.rotatePerSecondVar = dictionary->valueForKey("rotatePerSecondVariance")->floatValue();

            } else {
                CCASSERT( false, "Invalid emitterType in config file");
                CC_BREAK_IF(true);
            }

            // life span
            _life = dictionary->valueForKey("particleLifespan")->floatValue();
            _lifeVar = dictionary->valueForKey("particleLifespanVariance")->floatValue();

            // emission Rate
            _emissionRate = _totalParticles / _life;

            //don't get the internal texture if a batchNode is used
            if (!_batchNode)
            {
                // Set a compatible default for the alpha transfer
                _opacityModifyRGB = false;

                // texture        
                // Try to get the texture from the cache
                std::string textureName = dictionary->valueForKey("textureFileName")->getCString();
                
                size_t rPos = textureName.rfind('/');
               
                if (rPos != string::npos)
                {
                    string textureDir = textureName.substr(0, rPos + 1);
                    
                    if (dirname != NULL && textureDir != dirname)
                    {
                        textureName = textureName.substr(rPos+1);
                        textureName = string(dirname) + textureName;
                    }
                }
                else
                {
                    if (dirname != NULL)
                    {
                        textureName = string(dirname) + textureName;
                    }
                }
                
                Texture2D *tex = NULL;
                
                if (textureName.length() > 0)
                {
                    // set not pop-up message box when load image failed
                    bool bNotify = FileUtils::getInstance()->isPopupNotify();
                    FileUtils::getInstance()->setPopupNotify(false);
                    tex = TextureCache::getInstance()->addImage(textureName.c_str());
                    // reset the value of UIImage notify
                    FileUtils::getInstance()->setPopupNotify(bNotify);
                }
                
                if (tex)
                {
                    setTexture(tex);
                }
                else
                {                        
                    const char *textureData = dictionary->valueForKey("textureImageData")->getCString();
                    CCASSERT(textureData, "");
                    
                    int dataLen = strlen(textureData);
                    if(dataLen != 0)
                    {
                        // if it fails, try to get it from the base64-gzipped data    
                        int decodeLen = base64Decode((unsigned char*)textureData, (unsigned int)dataLen, &buffer);
                        CCASSERT( buffer != NULL, "CCParticleSystem: error decoding textureImageData");
                        CC_BREAK_IF(!buffer);
                        
                        int deflatedLen = ZipUtils::ccInflateMemory(buffer, decodeLen, &deflated);
                        CCASSERT( deflated != NULL, "CCParticleSystem: error ungzipping textureImageData");
                        CC_BREAK_IF(!deflated);
                        
                        // For android, we should retain it in VolatileTexture::addImage which invoked in TextureCache::getInstance()->addUIImage()
                        image = new Image();
                        bool isOK = image->initWithImageData(deflated, deflatedLen);
                        CCASSERT(isOK, "CCParticleSystem: error init image with Data");
                        CC_BREAK_IF(!isOK);
                        
                        setTexture(TextureCache::getInstance()->addUIImage(image, textureName.c_str()));

                        image->release();
                    }
                }
                CCASSERT( this->_texture != NULL, "CCParticleSystem: error loading the texture");
            }
            bRet = true;
        }
    } while (0);
    CC_SAFE_DELETE_ARRAY(buffer);
    CC_SAFE_DELETE_ARRAY(deflated);
    return bRet;
}

bool ParticleSystem::initWithTotalParticles(unsigned int numberOfParticles)
{
    _totalParticles = numberOfParticles;

    CC_SAFE_FREE(_particles);
    
    _particles = (tParticle*)calloc(_totalParticles, sizeof(tParticle));

    if( ! _particles )
    {
        CCLOG("Particle system: not enough memory");
        this->release();
        return false;
    }
    _allocatedParticles = numberOfParticles;

    if (_batchNode)
    {
        for (unsigned int i = 0; i < _totalParticles; i++)
        {
            _particles[i].atlasIndex=i;
        }
    }
    // default, active
    _isActive = true;

    // default blend function
    _blendFunc.src = CC_BLEND_SRC;
    _blendFunc.dst = CC_BLEND_DST;

    // default movement type;
    _positionType = POSITION_TYPE_FREE;

    // by default be in mode A:
    _emitterMode = MODE_GRAVITY;

    // default: modulate
    // XXX: not used
    //    colorModulate = YES;

    _isAutoRemoveOnFinish = false;

    // Optimization: compile updateParticle method
    //updateParticleSel = @selector(updateQuadWithParticle:newPosition:);
    //updateParticleImp = (CC_UPDATE_PARTICLE_IMP) [self methodForSelector:updateParticleSel];
    //for batchNode
    _transformSystemDirty = false;
    // update after action in run!
    this->scheduleUpdateWithPriority(1);

    return true;
}

ParticleSystem::~ParticleSystem()
{
    // Since the scheduler retains the "target (in this case the ParticleSystem)
	// it is not needed to call "unscheduleUpdate" here. In fact, it will be called in "cleanup"
    //unscheduleUpdate();
    CC_SAFE_FREE(_particles);
    CC_SAFE_RELEASE(_texture);
}

bool ParticleSystem::addParticle()
{
    if (this->isFull())
    {
        return false;
    }

    tParticle * particle = &_particles[ _particleCount ];
    this->initParticle(particle);
    ++_particleCount;

    return true;
}

void ParticleSystem::initParticle(tParticle* particle)
{
    // timeToLive
    // no negative life. prevent division by 0
    particle->timeToLive = _life + _lifeVar * CCRANDOM_MINUS1_1();
    particle->timeToLive = MAX(0, particle->timeToLive);

    // position
    particle->pos.x = _sourcePosition.x + _posVar.x * CCRANDOM_MINUS1_1();

    particle->pos.y = _sourcePosition.y + _posVar.y * CCRANDOM_MINUS1_1();


    // Color
    Color4F start;
    start.r = clampf(_startColor.r + _startColorVar.r * CCRANDOM_MINUS1_1(), 0, 1);
    start.g = clampf(_startColor.g + _startColorVar.g * CCRANDOM_MINUS1_1(), 0, 1);
    start.b = clampf(_startColor.b + _startColorVar.b * CCRANDOM_MINUS1_1(), 0, 1);
    start.a = clampf(_startColor.a + _startColorVar.a * CCRANDOM_MINUS1_1(), 0, 1);

    Color4F end;
    end.r = clampf(_endColor.r + _endColorVar.r * CCRANDOM_MINUS1_1(), 0, 1);
    end.g = clampf(_endColor.g + _endColorVar.g * CCRANDOM_MINUS1_1(), 0, 1);
    end.b = clampf(_endColor.b + _endColorVar.b * CCRANDOM_MINUS1_1(), 0, 1);
    end.a = clampf(_endColor.a + _endColorVar.a * CCRANDOM_MINUS1_1(), 0, 1);

    particle->color = start;
    particle->deltaColor.r = (end.r - start.r) / particle->timeToLive;
    particle->deltaColor.g = (end.g - start.g) / particle->timeToLive;
    particle->deltaColor.b = (end.b - start.b) / particle->timeToLive;
    particle->deltaColor.a = (end.a - start.a) / particle->timeToLive;

    // size
    float startS = _startSize + _startSizeVar * CCRANDOM_MINUS1_1();
    startS = MAX(0, startS); // No negative value

    particle->size = startS;

    if (_endSize == START_SIZE_EQUAL_TO_END_SIZE)
    {
        particle->deltaSize = 0;
    }
    else
    {
        float endS = _endSize + _endSizeVar * CCRANDOM_MINUS1_1();
        endS = MAX(0, endS); // No negative values
        particle->deltaSize = (endS - startS) / particle->timeToLive;
    }

    // rotation
    float startA = _startSpin + _startSpinVar * CCRANDOM_MINUS1_1();
    float endA = _endSpin + _endSpinVar * CCRANDOM_MINUS1_1();
    particle->rotation = startA;
    particle->deltaRotation = (endA - startA) / particle->timeToLive;

    // position
    if (_positionType == POSITION_TYPE_FREE)
    {
        particle->startPos = this->convertToWorldSpace(Point::ZERO);
    }
    else if (_positionType == POSITION_TYPE_RELATIVE)
    {
        particle->startPos = _position;
    }

    // direction
    float a = CC_DEGREES_TO_RADIANS( _angle + _angleVar * CCRANDOM_MINUS1_1() );    

    // Mode Gravity: A
    if (_emitterMode == MODE_GRAVITY)
    {
        Point v(cosf( a ), sinf( a ));
        float s = modeA.speed + modeA.speedVar * CCRANDOM_MINUS1_1();

        // direction
        particle->modeA.dir = v * s ;

        // radial accel
        particle->modeA.radialAccel = modeA.radialAccel + modeA.radialAccelVar * CCRANDOM_MINUS1_1();
 

        // tangential accel
        particle->modeA.tangentialAccel = modeA.tangentialAccel + modeA.tangentialAccelVar * CCRANDOM_MINUS1_1();

        // rotation is dir
        if(modeA.rotationIsDir)
            particle->rotation = -CC_RADIANS_TO_DEGREES(particle->modeA.dir.getAngle());
    }

    // Mode Radius: B
    else 
    {
        // Set the default diameter of the particle from the source position
        float startRadius = modeB.startRadius + modeB.startRadiusVar * CCRANDOM_MINUS1_1();
        float endRadius = modeB.endRadius + modeB.endRadiusVar * CCRANDOM_MINUS1_1();

        particle->modeB.radius = startRadius;

        if (modeB.endRadius == START_RADIUS_EQUAL_TO_END_RADIUS)
        {
            particle->modeB.deltaRadius = 0;
        }
        else
        {
            particle->modeB.deltaRadius = (endRadius - startRadius) / particle->timeToLive;
        }

        particle->modeB.angle = a;
        particle->modeB.degreesPerSecond = CC_DEGREES_TO_RADIANS(modeB.rotatePerSecond + modeB.rotatePerSecondVar * CCRANDOM_MINUS1_1());
    }    
}

void ParticleSystem::stopSystem()
{
    _isActive = false;
    _elapsed = _duration;
    _emitCounter = 0;
}

void ParticleSystem::resetSystem()
{
    _isActive = true;
    _elapsed = 0;
    for (_particleIdx = 0; _particleIdx < _particleCount; ++_particleIdx)
    {
        tParticle *p = &_particles[_particleIdx];
        p->timeToLive = 0;
    }
}
bool ParticleSystem::isFull()
{
    return (_particleCount == _totalParticles);
}

// ParticleSystem - MainLoop
void ParticleSystem::update(float dt)
{
    CC_PROFILER_START_CATEGORY(kProfilerCategoryParticles , "CCParticleSystem - update");

    if (_isActive && _emissionRate)
    {
        float rate = 1.0f / _emissionRate;
        //issue #1201, prevent bursts of particles, due to too high emitCounter
        if (_particleCount < _totalParticles)
        {
            _emitCounter += dt;
        }
        
        while (_particleCount < _totalParticles && _emitCounter > rate) 
        {
            this->addParticle();
            _emitCounter -= rate;
        }

        _elapsed += dt;
        if (_duration != -1 && _duration < _elapsed)
        {
            this->stopSystem();
        }
    }

    _particleIdx = 0;

    Point currentPosition = Point::ZERO;
    if (_positionType == POSITION_TYPE_FREE)
    {
        currentPosition = this->convertToWorldSpace(Point::ZERO);
    }
    else if (_positionType == POSITION_TYPE_RELATIVE)
    {
        currentPosition = _position;
    }

    if (_visible)
    {
        while (_particleIdx < _particleCount)
        {
            tParticle *p = &_particles[_particleIdx];

            // life
            p->timeToLive -= dt;

            if (p->timeToLive > 0) 
            {
                // Mode A: gravity, direction, tangential accel & radial accel
                if (_emitterMode == MODE_GRAVITY)
                {
                    Point tmp, radial, tangential;

                    radial = Point::ZERO;
                    // radial acceleration
                    if (p->pos.x || p->pos.y)
                    {
                        radial = p->pos.normalize();
                    }
                    tangential = radial;
                    radial = radial * p->modeA.radialAccel;

                    // tangential acceleration
                    float newy = tangential.x;
                    tangential.x = -tangential.y;
                    tangential.y = newy;
                    tangential = tangential * p->modeA.tangentialAccel;

                    // (gravity + radial + tangential) * dt
                    tmp = radial + tangential + modeA.gravity;
                    tmp = tmp * dt;
                    p->modeA.dir = p->modeA.dir + tmp;
                    tmp = p->modeA.dir * dt;
                    p->pos = p->pos + tmp;
                }

                // Mode B: radius movement
                else 
                {                
                    // Update the angle and radius of the particle.
                    p->modeB.angle += p->modeB.degreesPerSecond * dt;
                    p->modeB.radius += p->modeB.deltaRadius * dt;

                    p->pos.x = - cosf(p->modeB.angle) * p->modeB.radius;
                    p->pos.y = - sinf(p->modeB.angle) * p->modeB.radius;
                }

                // color
                p->color.r += (p->deltaColor.r * dt);
                p->color.g += (p->deltaColor.g * dt);
                p->color.b += (p->deltaColor.b * dt);
                p->color.a += (p->deltaColor.a * dt);

                // size
                p->size += (p->deltaSize * dt);
                p->size = MAX( 0, p->size );

                // angle
                p->rotation += (p->deltaRotation * dt);

                //
                // update values in quad
                //

                Point    newPos;

                if (_positionType == POSITION_TYPE_FREE || _positionType == POSITION_TYPE_RELATIVE)
                {
                    Point diff = currentPosition - p->startPos;
                    newPos = p->pos - diff;
                } 
                else
                {
                    newPos = p->pos;
                }

                // translate newPos to correct position, since matrix transform isn't performed in batchnode
                // don't update the particle with the new position information, it will interfere with the radius and tangential calculations
                if (_batchNode)
                {
                    newPos.x+=_position.x;
                    newPos.y+=_position.y;
                }

                updateQuadWithParticle(p, newPos);
                //updateParticleImp(self, updateParticleSel, p, newPos);

                // update particle counter
                ++_particleIdx;
            } 
            else 
            {
                // life < 0
                int currentIndex = p->atlasIndex;
                if( _particleIdx != _particleCount-1 )
                {
                    _particles[_particleIdx] = _particles[_particleCount-1];
                }
                if (_batchNode)
                {
                    //disable the switched particle
                    _batchNode->disableParticle(_atlasIndex+currentIndex);

                    //switch indexes
                    _particles[_particleCount-1].atlasIndex = currentIndex;
                }


                --_particleCount;

                if( _particleCount == 0 && _isAutoRemoveOnFinish )
                {
                    this->unscheduleUpdate();
                    _parent->removeChild(this, true);
                    return;
                }
            }
        } //while
        _transformSystemDirty = false;
    }
    if (! _batchNode)
    {
        postStep();
    }

    CC_PROFILER_STOP_CATEGORY(kProfilerCategoryParticles , "CCParticleSystem - update");
}

void ParticleSystem::updateWithNoTime(void)
{
    this->update(0.0f);
}

void ParticleSystem::updateQuadWithParticle(tParticle* particle, const Point& newPosition)
{
    CC_UNUSED_PARAM(particle);
    CC_UNUSED_PARAM(newPosition);
    // should be overridden
}

void ParticleSystem::postStep()
{
    // should be overridden
}

// ParticleSystem - Texture protocol
void ParticleSystem::setTexture(Texture2D* var)
{
    if (_texture != var)
    {
        CC_SAFE_RETAIN(var);
        CC_SAFE_RELEASE(_texture);
        _texture = var;
        updateBlendFunc();
    }
}

void ParticleSystem::updateBlendFunc()
{
    CCASSERT(! _batchNode, "Can't change blending functions when the particle is being batched");

    if(_texture)
    {
        bool premultiplied = _texture->hasPremultipliedAlpha();
        
        _opacityModifyRGB = false;
        
        if( _texture && ( _blendFunc.src == CC_BLEND_SRC && _blendFunc.dst == CC_BLEND_DST ) )
        {
            if( premultiplied )
            {
                _opacityModifyRGB = true;
            }
            else
            {
                _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
            }
        }
    }
}

Texture2D * ParticleSystem::getTexture() const
{
    return _texture;
}

// ParticleSystem - Additive Blending
void ParticleSystem::setBlendAdditive(bool additive)
{
    if( additive )
    {
        _blendFunc = BlendFunc::ADDITIVE;
    }
    else
    {
        if( _texture && ! _texture->hasPremultipliedAlpha() )
            _blendFunc = BlendFunc::ALPHA_NON_PREMULTIPLIED;
        else 
            _blendFunc = BlendFunc::ALPHA_PREMULTIPLIED;
    }
}

bool ParticleSystem::isBlendAdditive() const
{
    return( _blendFunc.src == GL_SRC_ALPHA && _blendFunc.dst == GL_ONE);
}

// ParticleSystem - Properties of Gravity Mode 
void ParticleSystem::setTangentialAccel(float t)
{
    CCASSERT( _emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.tangentialAccel = t;
}

float ParticleSystem::getTangentialAccel() const
{
    CCASSERT( _emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.tangentialAccel;
}

void ParticleSystem::setTangentialAccelVar(float t)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.tangentialAccelVar = t;
}

float ParticleSystem::getTangentialAccelVar() const
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.tangentialAccelVar;
}    

void ParticleSystem::setRadialAccel(float t)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.radialAccel = t;
}

float ParticleSystem::getRadialAccel() const
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.radialAccel;
}

void ParticleSystem::setRadialAccelVar(float t)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.radialAccelVar = t;
}

float ParticleSystem::getRadialAccelVar() const
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.radialAccelVar;
}

void ParticleSystem::setRotationIsDir(bool t)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.rotationIsDir = t;
}

bool ParticleSystem::getRotationIsDir() const
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.rotationIsDir;
}

void ParticleSystem::setGravity(const Point& g)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.gravity = g;
}

const Point& ParticleSystem::getGravity()
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.gravity;
}

void ParticleSystem::setSpeed(float speed)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.speed = speed;
}

float ParticleSystem::getSpeed() const
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.speed;
}

void ParticleSystem::setSpeedVar(float speedVar)
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    modeA.speedVar = speedVar;
}

float ParticleSystem::getSpeedVar() const
{
    CCASSERT(_emitterMode == MODE_GRAVITY, "Particle Mode should be Gravity");
    return modeA.speedVar;
}

// ParticleSystem - Properties of Radius Mode
void ParticleSystem::setStartRadius(float startRadius)
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    modeB.startRadius = startRadius;
}

float ParticleSystem::getStartRadius() const
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    return modeB.startRadius;
}

void ParticleSystem::setStartRadiusVar(float startRadiusVar)
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    modeB.startRadiusVar = startRadiusVar;
}

float ParticleSystem::getStartRadiusVar() const
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    return modeB.startRadiusVar;
}

void ParticleSystem::setEndRadius(float endRadius)
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    modeB.endRadius = endRadius;
}

float ParticleSystem::getEndRadius() const
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    return modeB.endRadius;
}

void ParticleSystem::setEndRadiusVar(float endRadiusVar)
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    modeB.endRadiusVar = endRadiusVar;
}

float ParticleSystem::getEndRadiusVar() const
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    return modeB.endRadiusVar;
}

void ParticleSystem::setRotatePerSecond(float degrees)
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    modeB.rotatePerSecond = degrees;
}

float ParticleSystem::getRotatePerSecond() const
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    return modeB.rotatePerSecond;
}

void ParticleSystem::setRotatePerSecondVar(float degrees)
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    modeB.rotatePerSecondVar = degrees;
}

float ParticleSystem::getRotatePerSecondVar() const
{
    CCASSERT(_emitterMode == MODE_RADIUS, "Particle Mode should be Radius");
    return modeB.rotatePerSecondVar;
}

bool ParticleSystem::isActive() const
{
    return _isActive;
}

unsigned int ParticleSystem::getTotalParticles() const
{
    return _totalParticles;
}

void ParticleSystem::setTotalParticles(unsigned int var)
{
    CCASSERT( var <= _allocatedParticles, "Particle: resizing particle array only supported for quads");
    _totalParticles = var;
}

const BlendFunc& ParticleSystem::getBlendFunc() const
{
    return _blendFunc;
}

void ParticleSystem::setBlendFunc(const BlendFunc &blendFunc)
{
    if( _blendFunc.src != blendFunc.src || _blendFunc.dst != blendFunc.dst ) {
        _blendFunc = blendFunc;
        this->updateBlendFunc();
    }
}

bool ParticleSystem::isAutoRemoveOnFinish() const
{
    return _isAutoRemoveOnFinish;
}

void ParticleSystem::setAutoRemoveOnFinish(bool var)
{
    _isAutoRemoveOnFinish = var;
}


// ParticleSystem - methods for batchNode rendering

ParticleBatchNode* ParticleSystem::getBatchNode(void) const
{
    return _batchNode;
}

void ParticleSystem::setBatchNode(ParticleBatchNode* batchNode)
{
    if( _batchNode != batchNode ) {

        _batchNode = batchNode; // weak reference

        if( batchNode ) {
            //each particle needs a unique index
            for (unsigned int i = 0; i < _totalParticles; i++)
            {
                _particles[i].atlasIndex=i;
            }
        }
    }
}

//don't use a transform matrix, this is faster
void ParticleSystem::setScale(float s)
{
    _transformSystemDirty = true;
    Node::setScale(s);
}

void ParticleSystem::setRotation(float newRotation)
{
    _transformSystemDirty = true;
    Node::setRotation(newRotation);
}

void ParticleSystem::setScaleX(float newScaleX)
{
    _transformSystemDirty = true;
    Node::setScaleX(newScaleX);
}

void ParticleSystem::setScaleY(float newScaleY)
{
    _transformSystemDirty = true;
    Node::setScaleY(newScaleY);
}


NS_CC_END

