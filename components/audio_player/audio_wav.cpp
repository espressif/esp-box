#include "sdkconfig.h"

#if defined(CONFIG_AUDIO_PLAYER_ENABLE_WAV)

#include <string.h>
#include "audio_wav.h"

static const char *TAG = "wav";

/**
 * @param fp
 * @param pInstance - Values can be considered valid if true is returned
 * @return true if file is a wav file
 */
bool is_wav(FILE *fp, wav_instance *pInstance) {
    fseek(fp, 0, SEEK_SET);

    size_t bytes_read = fread(&pInstance->header, 1, sizeof(wav_header_t), fp);
    if(bytes_read != sizeof(wav_header_t)) {
        return false;
    }

    wav_header_t *wav_head = &pInstance->header;
    if((NULL == strstr((char *)wav_head->ChunkID, "RIFF")) ||
        (NULL == strstr((char*)wav_head->Format, "WAVE"))
      )
    {
        return false;
    }

    // decode chunks until we find the 'data' one
    wav_subchunk_header_t subchunk;
    while(true) {
        bytes_read = fread(&subchunk, 1, sizeof(wav_subchunk_header_t), fp);
        if(bytes_read != sizeof(wav_subchunk_header_t)) {
            return false;
        }

        if(memcmp(subchunk.SubchunkID, "data", 4) == 0)
        {
            break;
        } else {
            // advance beyond this subchunk, it could be a 'LIST' chunk with file info or some other unhandled subchunk
            fseek(fp, subchunk.SubchunkSize, SEEK_CUR);
        }
    }

    LOGI_2("sample_rate=%d, channels=%d, bps=%d",
            wav_head->SampleRate,
            wav_head->NumChannels,
            wav_head->BitsPerSample);

    return true;
}

/**
 * @return true if data remains, false on error or end of file
 */
DECODE_STATUS decode_wav(FILE *fp, decode_data *pData, wav_instance *pInstance) {
    // read an even multiple of frames that can fit into output_samples buffer, otherwise
    // we would have to manage what happens with partial frames in the output buffer
    size_t bytes_per_frame = (pInstance->header.BitsPerSample / BITS_PER_BYTE) * pInstance->header.NumChannels;
    size_t frames_to_read = pData->samples_capacity / bytes_per_frame;
    size_t bytes_to_read = frames_to_read * bytes_per_frame;

    size_t bytes_read = fread(pData->samples, 1, bytes_to_read, fp);

    pData->fmt.channels = pInstance->header.NumChannels;
    pData->fmt.bits_per_sample = pInstance->header.BitsPerSample;
    pData->fmt.sample_rate = pInstance->header.SampleRate;

    if(bytes_read != 0)
    {
        pData->frame_count = (bytes_read / (pInstance->header.BitsPerSample / BITS_PER_BYTE)) / pInstance->header.NumChannels;
    } else {
        pData->frame_count = 0;
    }

    LOGI_2("bytes_per_frame %d, bytes_to_read %d, bytes_read %d, frame_count %d",
            bytes_per_frame, bytes_to_read, bytes_read,
            pData->frame_count);

    return (bytes_read == 0) ? DECODE_STATUS_DONE : DECODE_STATUS_CONTINUE;
}
#endif
