#include "main.h"

struct hvl_tune *ht = NULL;
int g_frequency;
DWORD g_remaining = 0;
char *g_rembuf = NULL;
uint8 *g_buffer = NULL;

void DLL_EXPORT mp_PluginInfo(struct mp_plugin_info *info)
{
    strcpy(info->name,"HVL/AHX plugin");
    info->version = 0x00000101;
}

bool DLL_EXPORT mp_Detect(char *filename, struct mp_song_info *songinfo)
{
    FILE *pFile;
    long lSize;

    hvl_InitReplayer();

    pFile = fopen ( filename , "rb" );
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    fseek( pFile, 0, SEEK_SET );
    g_buffer = (uint8*)malloc(lSize * sizeof(uint8));
    fread (g_buffer,1,lSize,pFile);

    if(!(ht = hvl_LoadTune( (uint8*)g_buffer,lSize, 44100, 0 )))
        return false;

    songinfo->format = (char*)malloc(strlen("Abyss Highest eXperience  ") * sizeof(char));
    switch(hvl_GetType())
    {
        case 0:
            strcpy(songinfo->format,"Abyss Highest eXperience");
            break;
        case 1:
            strcpy(songinfo->format,"Hively Tracker");
            break;
    }
    songinfo->name = (char*)malloc(strlen(ht->ht_Name) + 1 * sizeof(char));
    strcpy(songinfo->name,ht->ht_Name);
    songinfo->artist = NULL;
    songinfo->duration = -1;
    songinfo->info = NULL;
    songinfo->subsongs = -1;
    songinfo->voices = ht->ht_Channels;
    songinfo->steps = ht->ht_PositionNr;

    free(g_buffer);
    g_buffer = NULL;

    hvl_FreeTune(ht);
    ht = NULL;

    return true;
}

bool DLL_EXPORT mp_InitPlugin(char* filename, int frequency, int bps, int channels)
{
    if(g_rembuf)
    {
        free(g_rembuf);
        g_rembuf = NULL;
        g_remaining = 0;
    }

    FILE * pFile;
    long lSize;

    hvl_InitReplayer();

    pFile = fopen ( filename , "rb" );
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    fseek( pFile, 0, SEEK_SET );
    g_buffer = (uint8*)malloc(lSize * sizeof(uint8));
    fread (g_buffer,1,lSize,pFile);
    if(!(ht = hvl_LoadTune( (uint8*)g_buffer,lSize, frequency, 0 )))
        return false;

    g_frequency = frequency;
    return true;
}

void DLL_EXPORT mp_GetSongInfo(struct mp_song_info *songinf)
{

}

DWORD DLL_EXPORT mp_GetPosition()
{
    return 0;
}

void DLL_EXPORT mp_SetPosition(DWORD pos)
{

}

DWORD DLL_EXPORT mp_NextSubsong()
{
    return -1; // Not supported
}

DWORD DLL_EXPORT mp_PreviousSubsong()
{
    return -1; // Not supported
}

DWORD DLL_EXPORT mp_FillBuffer(void *buffer, DWORD length)
{
    int workbuf_length;

    if(g_remaining < length)
        workbuf_length = g_remaining + ceil((float)length / (float)((g_frequency*2*2)/50)) * ((g_frequency*2*2)/50);
    else
        workbuf_length = g_remaining;

    char *workbuf = (char*)malloc(workbuf_length);

    // Copy the remaining bytes from last time over to buffer
    if(g_remaining)
    {
        memcpy(workbuf,g_rembuf,g_remaining);
        free(g_rembuf);
        g_rembuf = NULL;
    }

    if(g_remaining < length)
    {
        for(int i = 0; i < ceil((float)length / (float)((g_frequency*2*2)/50)); i++)
        {
            hvl_DecodeFrame(ht,(i * ((g_frequency*2*2)/50)) + g_remaining + workbuf,(i * ((g_frequency*2*2)/50)) + g_remaining + workbuf + 2,4);
        }
    }

    memcpy(buffer,workbuf,length);

    // The remaining number of bytes that didn't fit in the requested buffer
    // we need to hold on to these until next time
    g_remaining = workbuf_length - length;

    g_rembuf = (char*)malloc(g_remaining * sizeof(char));
    memcpy(g_rembuf,length + workbuf, g_remaining);
    free(workbuf);

    return length;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}
