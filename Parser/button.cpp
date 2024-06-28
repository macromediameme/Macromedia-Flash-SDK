//
// This code snippet correctly parses DefineButton and DefineButton2
// It also include a some bug fixes in ParseDoAction.
//


void CInputScript::ParseButtonRecord(char *str, U32 iByte, BOOL fGetColorMatrix)
{
    U32 iPad = iByte >> 4;
    U32 iButtonStateHitTest = (iByte & 0x8);
    U32 iButtonStateDown = (iByte & 0x4);
    U32 iButtonStateOver = (iByte & 0x2);
    U32 iButtonStateUp = (iByte & 0x1);

    U32 iButtonCharacter = (U32)GetWord();
    U32 iButtonLayer = (U32)GetWord();

    MATRIX matrix;
    GetMatrix(&matrix);
    //PrintMatrix(matrix, str);

    if (fGetColorMatrix)
    {
        // nCharactersInButton always seems to be one (?)
        int nCharactersInButton = 1;

        for (int i=0; i<nCharactersInButton; i++)
        {
            CXFORM cxform;
            GetCxform(&cxform, true);   // ??could be false here??
        }
    }
}

void CInputScript::ParseDefineButton(char *str)
{
    U32 tagid = (U32) GetWord();

    U32 iButtonEnd = (U32)GetByte();
    do
        ParseButtonRecord(str, iButtonEnd, false);
    while ((iButtonEnd = (U32)GetByte()) != 0);

    // parse ACTIONRECORDs until ActionEndFlag
    ParseDoAction(str, false);
}

void CInputScript::ParseDefineButton2(char *str)
{
    U32 tagid = (U32) GetWord();

    printf("%stagDefineButton2 \ttagid %-5u\n", str, tagid);

    U32 iTrackAsMenu = (U32) GetByte();

    // Get offset to first "Button2ActionCondition"
    // This offset is not in the spec!
    U32 iOffset = (U32) GetWord();
    U32 iNextAction = m_filePos + iOffset - 2;

    //
    // Parse Button Records
    //

    U32 iButtonEnd = (U32)GetByte();
    do
        ParseButtonRecord(str, iButtonEnd, true);
    while ((iButtonEnd = (U32)GetByte()) != 0);

    //
    // Parse Button2ActionConditions
    //

    m_filePos = iNextAction;

    U32 iActionOffset = 0;
    while (true)
    {
        iActionOffset = (U32) GetWord();
        iNextAction  = m_filePos + iActionOffset - 2;

        U32 iCondition = (U32) GetWord();

        // parse ACTIONRECORDs until ActionEndFlag
        ParseDoAction(str, false);

        // Action Offset of zero means there's no more
        if (iActionOffset == 0)
            break;

        m_filePos = iNextAction;
    }
}

void CInputScript::ParseDoAction(char *str, BOOL fPrintTag)
{
    if (fPrintTag)
    {
        printf("%stagDoAction\n",  str);
    }

    for (;;) 
    {
        // Handle the action
        int actionCode = GetByte();
        INDENT;
        printf("%saction code 0x%02x ", str, actionCode);
        if (actionCode == 0)
        {
            // Action code of zero indicates end of actions
            return;
        }

        int len = 0;
        if (actionCode & sactionHasLength) 
        {
            len = GetWord();
            printf("has length %5u ", len);
        }        

        S32 pos = m_filePos + len;

        switch ( actionCode ) 
        {
            // Handle timeline actions
            case sactionGotoFrame:
                printf("gotoFrame %5u\n", GetWord());
                break;

            case sactionGotoLabel: 
                printf("gotoLabel %s\n", &m_fileBuf[m_filePos]);    // swfparse used to crash here!
                break;
                
            case sactionNextFrame:
                printf("gotoNextFrame\n");
                break;

            case sactionPrevFrame:
                printf("gotoPrevFrame\n");
                break;

            case sactionPlay:
                printf("play\n");
                break;

            case sactionStop:
                printf("stop\n");
                break;

            case sactionWaitForFrame:
            {
                int frame = GetWord();
                int skipCount = GetByte();
                printf("waitForFrame %-5u skipCount %-5u\n", frame, skipCount);
                break;
            }

            case sactionGetURL: {
                char *url = GetString();
                char *target = GetString();
                printf("getUrl %s target %s\n", url, target);
                } break;

            case sactionSetTarget:
                printf("setTarget %s\n", &m_fileBuf[m_filePos]);        // swfparse used to crash here!
                break;

            case sactionStopSounds:
                printf("stopSounds\n");
                break;

            case sactionToggleQuality:
                printf("toggleQuality\n");
                break;
        }
        m_filePos = pos;
    }
}
