#include <string.h>
#include <stdio.h>
#include "audio_wav.h"

static const char *TAG = "wav";

/**
 * @param stream
 * @param pInstance - Values can be considered valid if true is returned
 * @return true if uint8_t is a wav uint8_t
 */
bool is_wav(uint8_t **stream, wav_instance *pInstance) {

    // assert(*stream);
    memcpy(&pInstance->header, *stream, sizeof(wav_header_t));
    size_t bytes_read = sizeof(wav_header_t);
    if(bytes_read != sizeof(wav_header_t)) {
        return false;
    }

    wav_header_t *wav_head = &pInstance->header;
    if((NULL == strstr(reinterpret_cast<char *>(wav_head->ChunkID), "RIFF")) ||
        (NULL == strstr(reinterpret_cast<char*>(wav_head->Format), "WAVE"))
      )
    {
        return false;
    }

    // decode chunks until we find the 'data' one
    wav_subchunk_header_t subchunk;
    do{
        // bytes_read = fread(&subchunk, 1, sizeof(wav_subchunk_header_t), stream);
        memcpy(&subchunk, *stream + sizeof(wav_header_t), sizeof(wav_subchunk_header_t));
        bytes_read = sizeof(wav_subchunk_header_t);

        if(bytes_read != sizeof(wav_subchunk_header_t)) {
            return false;
        }

        if(memcmp(subchunk.SubchunkID, "data", 4) == 0)
        {
            // advance beyond this subchunk, it could be a 'LIST' chunk with uint8_t info or some other unhandled subchunk
            // fseek(stream, subchunk.SubchunkSize, SEEK_CUR);
            (*stream) += (sizeof(wav_subchunk_header_t) + sizeof(wav_header_t));
        }
    }while(false);

    LOGI_2("sample_rate=%d, channels=%d, bps=%d",
            wav_head->SampleRate,
            wav_head->NumChannels,
            wav_head->BitsPerSample);

    return true;
}

/**
 * @return true if data remains, false on error or end of uint8_t
 */
DECODE_STATUS decode_wav(uint8_t **stream, uint8_t *base, uint32_t stream_len, decode_data *pData, wav_instance *pInstance) {
    // read an even multiple of frames that can fit into output_samples buffer, otherwise
    // we would have to manage what happens with partial frames in the output buffer
    size_t bytes_per_frame = (pInstance->header.BitsPerSample / BITS_PER_BYTE) * pInstance->header.NumChannels;
    size_t frames_to_read = pData->samples_capacity / bytes_per_frame;
    size_t bytes_to_read = frames_to_read * bytes_per_frame;

    // size_t bytes_read = fread(pData->samples, 1, bytes_to_read, stream);
    size_t bytes_read;
    if((*stream - base) < stream_len){
        if((*stream + bytes_to_read) > (base + stream_len)){
            bytes_read = (base + stream_len) - *stream;
        }
        else{
            bytes_read = bytes_to_read;
        }
    }
    else{
        bytes_read = 0;
    }

    if(bytes_read){
        memcpy(pData->samples, *stream, bytes_read);
    }
    (*stream) +=bytes_read;

    pData->fmt.channels = pInstance->header.NumChannels;
    pData->fmt.bits_per_sample = pInstance->header.BitsPerSample;
    pData->fmt.sample_rate = pInstance->header.SampleRate;

    if(bytes_read != 0)
    {
        pData->frame_count = (bytes_read / (pInstance->header.BitsPerSample / BITS_PER_BYTE)) / pInstance->header.NumChannels;
    } else {
        pData->frame_count = 0;
    }

    LOGI_2("%p, bytes_per_frame %d, bytes_to_read %d, bytes_read %d, frame_count %d",
            *stream,
            bytes_per_frame, bytes_to_read, bytes_read,
            pData->frame_count);

    return (bytes_read == 0) ? DECODE_STATUS_DONE : DECODE_STATUS_CONTINUE;
}
