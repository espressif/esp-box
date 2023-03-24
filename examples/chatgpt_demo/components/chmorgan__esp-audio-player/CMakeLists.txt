
set(srcs
    "audio_player.cpp"
)

set(includes
    "include"
)

set(requires "")

if(CONFIG_AUDIO_PLAYER_ENABLE_MP3)
    list(APPEND srcs "audio_mp3.cpp")
endif()

# TODO: move inside of the 'if(CONFIG_AUDIO_PLAYER_ENABLE_MP3)' when everything builds correctly
list(APPEND requires "esp-libhelix-mp3")

if(CONFIG_AUDIO_PLAYER_ENABLE_WAV)
    list(APPEND srcs "audio_wav.cpp")
endif()

idf_component_register(SRCS "${srcs}"
                       REQUIRES "${requires}"
                       INCLUDE_DIRS "${includes}"
                       REQUIRES driver
)
