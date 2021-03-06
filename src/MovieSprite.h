#pragma once
#include "Sprite.h"
#include "MemoryTexture.h"
#include "core/Mutex.h"
#include <string>


namespace oxygine
{
    DECLARE_SMART(MovieSprite, spMovieSprite);

    class UberShaderProgram;

    /**
        MovieSprite uses libVLC. That means it need access to VLC player dlls.
        there are 2 solutions:
        1.  You could just copy folder "plugins" from
                c:\Program Files (x86)\VideoLAN\VLC\plugins\
            to working directory (data folder)
                data\
                    \plugins
        2.  Or you will need to define the environment variable VLC_PLUGIN_PATH pointing to VLC modules located in vlc-src/modules.

    */
    class MovieSprite: public Sprite
    {
    public:
        /**
        when highQualityShader==false then easy YUV to RGB conversion used in pixel shader
        */
        static void init(bool highQualityShader = false);
        static void free();
        static spMovieSprite create();

        //event dispatched when movie is completed
        enum { COMPLETE = Event::COMPLETE };

        MovieSprite();
        ~MovieSprite();

        /**Play video. Position and Size should be set Before play*/
        void play();
        void pause();
        void stop();

        bool isPlaying() const;


        /**Change internal texture format. Should be called before Play. Supported TF_R8G8BA8 and TF_R5G6B5, default: TF_R5G6B5*/
        //void setFormat(TextureFormat tf);

        /**Changes current movie*/
        void setMovie(const std::string& movie, bool hasAlphaChannel = false);

        /**Changes size of texture where video is rendering. Default value is equal to original movie dimension. Should be called before Play*/
        //void setBufferSize(const Point& size);

        /**Set volume: [0-100]*/
        void setVolume(int v);

        void setLooped(bool);

        /**Returns original movie dimension*/
        Point getMovieSize() const;

    protected:
        void initPlayer();
        void clear();

        void asyncDone();

        virtual void _initPlayer() = 0;
        virtual void _play() = 0;
        virtual void _pause() = 0;
        virtual void _resume() = 0;
        virtual void _setVolume(float v) = 0;
        virtual void _stop() = 0;
        virtual bool _isPlaying() const = 0;
        virtual void _clear() = 0;
        virtual void _update(const UpdateState&) = 0;
        //virtual Point _getMovieSize() const = 0;


        void doRender(const RenderState& rs) OVERRIDE;
        void doUpdate(const UpdateState& us) OVERRIDE;

        void convert();

        Mutex _mutex;

        Point _bufferSize;
        Rect _movieRect;

        MemoryTexture _mtUV;
        MemoryTexture _mtYA;
        spNativeTexture _textureUV;
        spNativeTexture _textureYA;
        Vector2     _yaScale;

        std::string _fname;

        bool _dirty;
        bool _ready;
        int _volume;

        bool _paused;
        bool _playing;

        bool _hasAlphaChannel;

        bool _initialized;
        bool _completeDispatched;
        bool _looped;

        static UberShaderProgram* _shader;
        void setUniforms(IVideoDriver* driver, ShaderProgram* prog);
    };

    DECLARE_SMART(MovieTween, spMovieTween);
    spMovieTween createMovieTween(const std::string& name, bool alpha = true);
}