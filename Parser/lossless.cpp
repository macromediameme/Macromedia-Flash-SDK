void CInputScript::ParseDefineBitsLossless(char *str)
{
    U32 tagid = (U32) GetWord();

    printf("%stagDefineBitsLossless tagid %-5u\n", str, tagid);
     
    int iFormat = GetByte();

    // We don't handle anything but 8-bit image data for now
    if (iFormat != 3)
        return;

    int iWidth  = GetWord();
    int iHeight = GetWord();
    int iTableSize = iTableSize = GetByte() + 1;

    // pbBuffer points to the current pos in the file buffer
    byte* pbBuffer = &m_fileBuf[m_filePos];

    // Allocate space for the color table
    byte* pbColorTable = new byte[iTableSize*3];

    //
    // Decompress the color table
    //

    // Set up ZLIB structure
    z_stream stream;
    stream.next_in = pbBuffer;
    stream.avail_in = 1;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.next_out = pbColorTable;
    stream.avail_out = iTableSize*3;

    // Initialize ZLIB decompressor
    inflateInit(&stream);

    // Read the colormap byte-by-byte
    int status;
    while (true)
    {
        status = inflate(&stream, Z_SYNC_FLUSH);
        if (status == Z_STREAM_END)
            break;
        if (status != Z_OK)
        {
            printf("Zlib cmap error : %s\n", stream.msg);
            return;
        }
        stream.avail_in = 1;

        if (stream.avail_out == 0)
            break;      // Colormap full

    }

    //
    // Decompress the image data
    //

    // Allocate space for the image data
    byte* pbData = new byte[iWidth*iHeight];

    stream.next_out = pbData;
    stream.avail_out = iWidth*iHeight;

    // Read the image data byte-by-byte
    while (true)
    {
        status = inflate(&stream, Z_SYNC_FLUSH) ;
        if (status == Z_STREAM_END)
            break;

        if (status != Z_OK)
        {
            printf("Zlib data error : %s\n", stream.msg);
            return;
        }
        stream.avail_in = 1;
    }

    // Tell ZLIB decompressor we're finished
    inflateEnd(&stream);

    //
    // do something with pbData and pbColorTable here!
    //

    delete pbData;
    delete pbColorTable;
}
