idf_component_register(SRCS "audio_player.cpp" "audio_mp3.cpp" "audio_wav.cpp"
                       INCLUDE_DIRS "include"
                       REQUIRES libhelix-mp3
)
