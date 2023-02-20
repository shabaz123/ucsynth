#ifndef WAV_HEADER_FILE
#define WAV_HEADER_FILE

typedef struct PCM16_stereo_s
{
    int16_t left;
    int16_t right;
} PCM16_stereo_t;

PCM16_stereo_t *allocate_PCM16_stereo_buffer(   int32_t FrameCount);

int write_PCM16_stereo_header(  FILE*   file_p,
                                int32_t SampleRate,
                                int32_t FrameCount);

size_t  write_PCM16wav_data(FILE*           file_p,
                            int32_t         FrameCount,
                            PCM16_stereo_t  *buffer_p);






#endif // WAV_HEADER_FILE

