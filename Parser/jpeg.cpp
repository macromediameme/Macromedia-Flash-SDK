////////////////////////////////////////////////////////////////////////////////

#define OUTPUT_BUF_SIZE 4096

typedef struct
{
  struct jpeg_destination_mgr mgr;  // Standard JPEG dest mgr.

  JOCTET*   pFileBuffer;            // SWF file buffer
  int       iFilePos;               // SWF file position
}
JpegDestMgr;


// Methods for Destination data manager
void init_destination (j_compress_ptr cinfo)
{
    JpegDestMgr* pDestMgr = (JpegDestMgr*)cinfo->dest;

    pDestMgr->mgr.next_output_byte = pDestMgr->pFileBuffer + pDestMgr->iFilePos;
    pDestMgr->mgr.free_in_buffer = OUTPUT_BUF_SIZE;
}

//
// Empty the output buffer --- called whenever buffer fills up.
//
// In typical applications, this should write the entire output buffer
// (ignoring the current state of next_output_byte & free_in_buffer),
// reset the pointer & count to the start of the buffer, and return TRUE
// indicating that the buffer has been dumped.
//

boolean empty_output_buffer (j_compress_ptr cinfo)
{
    JpegDestMgr* pDestMgr = (JpegDestMgr*)cinfo->dest;

    pDestMgr->iFilePos += OUTPUT_BUF_SIZE;
    pDestMgr->mgr.next_output_byte = pDestMgr->pFileBuffer + pDestMgr->iFilePos;
    pDestMgr->mgr.free_in_buffer = OUTPUT_BUF_SIZE;

    return TRUE;
}

// Terminate destination --- called by jpeg_finish_compress
void term_destination (j_compress_ptr cinfo)
{
    JpegDestMgr* pDestMgr = (JpegDestMgr*)cinfo->dest;
    pDestMgr->iFilePos += (OUTPUT_BUF_SIZE - pDestMgr->mgr.free_in_buffer);
}

////////////////////////////////////////////////////////////////////////////////

int CSwfJpeg::PutInterchangeBits(byte* pbFileBuffer, int iFilePos)
{
    // First, make sure we have this image in RGB mode
    IndexedToRGB();

    // Step 1: allocate and initialize JPEG compression object

    // We have to set up the error handler first, in case the initialization
    // step fails.  (Unlikely, but it could happen if you are out of memory.)
    // This routine fills in the contents of struct jerr, and returns jerr's
    // address which we place into the link field in cinfo.

    struct jpeg_compress_struct cinfo;

    // Initialise error manager
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);

    // Now we can initialize the JPEG compression object.
    jpeg_create_compress(&cinfo);

    // Step 2: specify data destination (eg, a file)
    JpegDestMgr destmgr;
    destmgr.mgr.init_destination = init_destination;
    destmgr.mgr.empty_output_buffer = empty_output_buffer;
    destmgr.mgr.term_destination = term_destination;

    destmgr.pFileBuffer = pbFileBuffer;
    destmgr.iFilePos = iFilePos;

    cinfo.dest = (jpeg_destination_mgr*)&destmgr;

    /* Step 3: set parameters for compression */

    // First we supply a description of the input image.
    // Four fields of the cinfo struct must be filled in:

    cinfo.image_width = m_iWidth;           // image width and height, in pixels
    cinfo.image_height = m_iHeight;
    cinfo.input_components = 3;             // # of color components per pixel
    cinfo.in_color_space = JCS_RGB;         // colorspace of input image

    // Now use the library's routine to set default compression parameters.
    // (You must set at least cinfo.in_color_space before calling this,
    //since the defaults depend on the source color space.)
    jpeg_set_defaults(&cinfo);

    // Now you can set any non-default parameters you wish to.
    // Here we just illustrate the use of quality (quantization table) scaling:
    jpeg_set_quality(&cinfo, 100, TRUE);

    // Write the JPEG tables.
    jpeg_write_tables(&cinfo);


    // Step 4: Start compressor
    jpeg_suppress_tables(&cinfo, TRUE);
    jpeg_start_compress(&cinfo, FALSE);

    // Step 5: write scanlines...
    JSAMPROW    jsrLine = (JSAMPROW)m_pbRGBPixels;
    int         iRGBLen = 3 * m_iWidth;

    for (int i=0; i<m_iHeight; i++, jsrLine += iRGBLen)
        (void) jpeg_write_scanlines(&cinfo, &jsrLine, 1);

    // Step 6: Finish compression
    jpeg_finish_compress(&cinfo);

    /// Step 7: release JPEG compression object
    jpeg_destroy_compress(&cinfo);

    // And we're done!
    return destmgr.iFilePos;
}

void CExportSwf::PutDefineBitsJPEG2(CSwfImage* pImage)
{
    PutTag(stagDefineBitsJPEG2);
    PutWord(pImage->m_iImageID);

    CSwfJpeg* pJpeg = new CSwfJpeg(pImage->m_iImageID);
    pJpeg->Copy(pImage);

    m_filePos = pJpeg->PutInterchangeBits(m_fileBuf, m_filePos);
    PutTagLen();
}