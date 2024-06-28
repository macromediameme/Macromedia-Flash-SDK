//
// This code snippet shows how to parse DefineFont,
// DefineFontInfo, TEXTRECORDs and DefineText
//

void CInputScript::ParseDefineFont(char *str)
{
    U32 iFontID = (U32)GetWord();
    printf("%stagDefineFont \t\tFont ID %-5u\n", str, iFontID);

    int iStart = m_filePos;

    int iOffset = GetWord();
    //printf("%s\tiOffset: 0x%04x\n", str, iOffset);

    int iGlyphCount = iOffset/2;
    m_iGlyphCounts[iFontID] = iGlyphCount;
    printf("%s\tnumber of glyphs: %d\n", str, iGlyphCount);

    int* piOffsetTable = new int[iGlyphCount];
    piOffsetTable[0] = iOffset;

    for(int n=1; n<iGlyphCount; n++)
        piOffsetTable[n] = GetWord();

    for(n=0; n<iGlyphCount; n++)
    {
        m_filePos = piOffsetTable[n] + iStart;

        InitBits(); // reset bit counter

        m_nFillBits = (U16) GetBits(4);
        m_nLineBits = (U16) GetBits(4);

        //printf("%s\tm_nFillBits:%d m_nLineBits:%d\n", str, m_nFillBits, m_nLineBits);

        int xLast = 0;
        int yLast = 0;

        BOOL fAtEnd = false;

        while (!fAtEnd)
            fAtEnd = ParseShapeRecord(str, xLast, yLast);
    }

    delete piOffsetTable;
}


void CInputScript::ParseDefineFontInfo(char *str)
{
    U32 iFontID = (U32) GetWord();
    printf("%stagDefineFontInfo \tFont ID %-5u\n", str, iFontID);

    int iNameLen = GetByte();
    char* pszName = new char[iNameLen+1];
    for(int n=0; n < iNameLen; n++)
        pszName[n] = (char)GetByte();
    pszName[n] = '\0';

    printf("%s\tFontName: '%s'\n",str, pszName);

    delete pszName;

    U8 flags = (FontFlags)GetByte();

    int iGlyphCount = m_iGlyphCounts[iFontID];

    int *piCodeTable = new int[iGlyphCount];

    printf("%s\t", str);
    for(n=0; n < iGlyphCount; n++)
    {
        if (flags & fontWideCodes)
            piCodeTable[n] = (int)GetWord();
        else
            piCodeTable[n] = (int)GetByte();
        printf("[%d,'%c'] ", piCodeTable[n], (char)piCodeTable[n]);
    }

    printf("\n\n");

    delete piCodeTable;
}

BOOL CInputScript::ParseTextRecord(char* str, int nGlyphBits, int nAdvanceBits)
{
    U8 flags = (TextFlags)GetByte();
    if (flags == 0) return 0;
    printf("\n%s\tflags: 0x%02x\n", str, flags);

    if (flags & isTextControl)
    {
        if (flags & textHasFont)
        {
            long fontId = GetWord();
            printf("%s\tfontId: %d\n", str, fontId);
        }
        if (flags & textHasColor)
        {
            int r = GetByte();
            int g = GetByte();
            int b = GetByte();
            printf("%s\tfontColour: (%d,%d,%d)\n", str, r, g, b);
        }
        if (flags & textHasXOffset)
        {
            int iXOffset = GetWord();
            printf("%s\tX-offset: %d\n", str, iXOffset);
        }
        if (flags & textHasYOffset)
        {
            int iYOffset = GetWord();
            printf("%s\tY-offset: %d\n", str, iYOffset);
        }
        if (flags & textHasFont)
        {
            int iFontHeight = GetWord();
            printf("%s\tFont Height: %d\n", str, iFontHeight);
        }
    }
    else
    {
        int iGlyphCount = flags;
        printf("%s\tnumber of glyphs: %d\n", str, iGlyphCount);

        InitBits();     // reset bit counter

        printf("%s\t", str);
        for (int g = 0; g < iGlyphCount; g++)
        {
            int iIndex = GetBits(nGlyphBits);
            int iAdvance = GetBits(nAdvanceBits);
            printf("[%d,%d] ", iIndex, iAdvance);
        }
        printf("\n");
    }

    return true;
}


void CInputScript::ParseDefineText(char *str)
{
    U32 tagid = (U32) GetWord();
    printf("%stagDefineText \t\ttagid %-5u\n", str, tagid);

    SRECT   rect;
    GetRect(&rect);
    //PrintRect(rect, str);

    MATRIX  m;
    GetMatrix(&m);
    //PrintMatrix(m, str);


    int nGlyphBits = (int)GetByte();
    int nAdvanceBits = (int)GetByte();

    printf("%s\tnGlyphBits:%d nAdvanceBits:%d\n", str, nGlyphBits, nAdvanceBits);

    BOOL fContinue = true;

    do
        fContinue = ParseTextRecord(str, nGlyphBits, nAdvanceBits);
    while (fContinue);

    printf("\n");
}
