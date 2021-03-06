#include "MovieSprite.h"

#include "core/oxygine.h"
#include "core/Mutex.h"
#include "core/ImageDataOperations.h"
#include "core/UberShaderProgram.h"
#include "RenderState.h"
#include "STDRenderer.h"
#include "Stage.h"
#include "core/ThreadMessages.h"

namespace oxygine
{
    UberShaderProgram* MovieSprite::_shader = 0;

    MovieSprite::MovieSprite() : _dirty(false),
        _completeDispatched(false),
        _bufferSize(0, 0),
        _movieRect(0, 0, 0, 0),
        _initialized(false), _volume(100), _paused(false), _playing(false),
        _looped(false),
        _ready(false),
        _hasAlphaChannel(false)
    {
    }

    MovieSprite::~MovieSprite()
    {
        if (_textureUV)
            _textureUV->release();
        if (_textureYA)
            _textureYA->release();
        //clear();
    }

    void MovieSprite::convert()
    {
        if (_dirty && _textureUV)
        {
            MutexAutoLock l(_mutex);
            _textureUV->updateRegion(0, 0, _mtUV.lock());
            _textureYA->updateRegion(0, 0, _mtYA.lock());
            _dirty = false;
            _ready = true;
        }
    }

    void MovieSprite::setUniforms(IVideoDriver* driver, ShaderProgram* prog)
    {
        driver->setUniform("yaScale", &_yaScale, 1);
    }

    void MovieSprite::doRender(const RenderState& rs)
    {
        convert();

        if (!_ready)
            return;

        _shader->setShaderUniformsCallback(CLOSURE(this, &MovieSprite::setUniforms));

#if OXYGINE_RENDERER > 2
        STDRenderer* renderer = STDRenderer::instance;
        renderer->setUberShaderProgram(_shader);
        Sprite::doRender(rs);
        renderer->setUberShaderProgram(&STDRenderer::uberShader);
#else
        STDRenderer* renderer = safeCast<STDRenderer*>(rs.renderer);
        renderer->setUberShaderProgram(_shader);
        Sprite::doRender(rs);
        renderer->setUberShaderProgram(&Renderer::uberShader);
#endif

        _shader->setShaderUniformsCallback(UberShaderProgram::ShaderUniformsCallback());

        //Sprite::doRender(rs);
    }

    /*
    void MovieSprite::setBufferSize(const Point& size)
    {
        _bufferSize = size;
    }
    */

    void MovieSprite::initPlayer()
    {
        if (_initialized)
            return;

        _initialized = true;

        _initPlayer();

        Point sz = _bufferSize;
        sz = Point(nextPOT(sz.x), nextPOT(sz.y));

        Point uvSize = _bufferSize / 2;
        uvSize = Point(nextPOT(uvSize.x), nextPOT(uvSize.y));
        //uvSize = sz;
        _mtUV.init(uvSize.x, uvSize.y, TF_A8L8);
        _mtUV.fill_zero();
        _textureUV = IVideoDriver::instance->createTexture();
        _textureUV->init(uvSize.x, uvSize.y, _mtUV.getFormat(), false);

        _mtYA.init(sz.x, sz.y, _hasAlphaChannel ? TF_A8L8 : TF_L8);
        _mtYA.fill_zero();
        _textureYA = IVideoDriver::instance->createTexture();
        _textureYA->init(sz.x, sz.y, _mtYA.getFormat(), false);

        if (_hasAlphaChannel)
            setBlendMode(blend_premultiplied_alpha);
        else
            setBlendMode(blend_disabled);

        Diffuse d;
        d.base = _textureYA;
        d.alpha = _textureUV;
        d.premultiplied = true;

        AnimationFrame frame;
        RectF mr = _movieRect.cast<RectF>();

        Vector2 szf = sz.cast<Vector2>();
        RectF tcYA = RectF(mr.pos.div(szf), mr.size.div(szf));
        frame.init(0, d, tcYA, mr, mr.size);

        _yaScale = Vector2(uvSize.x / szf.x, uvSize.y / szf.y);
        _yaScale = Vector2(0.5f, 0.5f);
        _yaScale = Vector2(1, 1);
        Vector2 s = getSize();
        setAnimFrame(frame);
        setSize(s);
    }

    void MovieSprite::doUpdate(const UpdateState& us)
    {
        _update(us);
    }

    void MovieSprite::setVolume(int v)
    {
        _volume = v;
        _setVolume(v);
    }

    void MovieSprite::setLooped(bool l)
    {
        _looped = l;
    }

    Point MovieSprite::getMovieSize() const
    {
        return _movieRect.getSize();
    }

    void MovieSprite::clear()
    {
        _clear();

        _dirty = false;
        _playing = false;
        _paused = false;
        _ready = false;

        if (_textureUV)
            _textureUV->release();
        _textureUV = 0;

        if (_textureYA)
            _textureYA->release();
        _textureYA = 0;

        _movieRect = Rect(0, 0, 0, 0);

        setAnimFrame(0);

        _initialized = false;

        {
            MutexAutoLock m(_mutex);

            _mtUV.cleanup();
            _mtYA.cleanup();
        }
    }

    void MovieSprite::setMovie(const std::string& movie, bool hasAlpha)
    {
        clear();
        _hasAlphaChannel = hasAlpha;
        _fname = movie;
        initPlayer();
    }

    void MovieSprite::play()
    {
        if (_playing)
        {
            if (_paused)
            {
                _resume();
                _paused = false;
                return;
            }

            return;
        }


        _playing = true;


        initPlayer();

        _completeDispatched = false;
        _play();
    }

    void MovieSprite::pause()
    {
        if (_paused)
            return;

        _pause();
        _paused = true;
    }

    void MovieSprite::stop()
    {
        clear();
    }

    bool MovieSprite::isPlaying() const
    {
        return _playing && !_paused;
    }

    void MovieSprite::asyncDone()
    {
        addRef();
        core::getMainThreadMessages().postCallback([ = ]()
        {
            _playing = false;

            Event ev(MovieSprite::COMPLETE);
            dispatchEvent(&ev);

            releaseRef();
        });
    }
}
