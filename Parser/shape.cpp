//
// This code snippet shows how to parse DefineShape,
// SHAPERECORDs, LINESTYLEs and FILLSTYLEs.
// It also includes a bug fix to ParseDefineShape.
//

BOOL CInputScript::ParseShapeRecord(char *str, int& xLast, int& yLast)
{
    // Determine if this is an edge.
    BOOL isEdge = (BOOL) GetBits(1);

    if (!isEdge)
    {
        // Handle a state change
        U16 flags = (U16) GetBits(5);

        // Are we at the end?
        if (flags == 0)
        {
            printf("\tEnd of shape.\n\n");
            return true;
        }

        // Process a move to.
        if (flags & eflagsMoveTo)
        {
            U16 nBits = (U16) GetBits(5);
            S32 x = GetSBits(nBits);
            S32 y = GetSBits(nBits);
            xLast = x;
            yLast = y;
            printf("\tmoveto: (%d,%d)\n", xLast, yLast);
        }
        // Get new fill info.
        if (flags & eflagsFill0)
        {
            int i = GetBits(m_nFillBits);
            printf("\tFillStyle0: %d\n", i);
        }
        if (flags & eflagsFill1)
        {
            int i = GetBits(m_nFillBits);
            printf("\tFillStyle1: %d (%d bits)\n", i, m_nFillBits);
        }
        // Get new line info
        if (flags & eflagsLine)
        {
            int i = GetBits(m_nLineBits);
            printf("\tLineStyle: %d\n", i);
        }
        // Check to get a new set of styles for a new shape layer.
        if (flags & eflagsNewStyles)
        {
            printf("\tFound more Style info\n");

            // Parse the style.
            ParseShapeStyle(str);

            // Reset.
            m_nFillBits = (U16) GetBits(4);
            m_nLineBits = (U16) GetBits(4);
        }
        if (flags & eflagsEnd)
        {
            printf("\tEnd of shape.\n\n");
        }
  
        return flags & eflagsEnd ? true : false;
    }
    else
    {
        if (GetBits(1))
        {
            // Handle a line
            U16 nBits = (U16) GetBits(4) + 2;   // nBits is biased by 2

            // Save the deltas
            if (GetBits(1))
            {
                // Handle a general line.
                S32 x = GetSBits(nBits);
                S32 y = GetSBits(nBits);
                xLast += x;
                yLast += y;

                printf("\tlineto: (%d,%d).\n", xLast, yLast);
            }
            else
            {
                // Handle a vert or horiz line.
                if (GetBits(1))
                {
                    // Vertical line
                    S32 y = GetSBits(nBits);
                    yLast += y;

                    printf("\tvlineto: (%d,%d).\n", xLast, yLast);
                }
                else
                {
                    // Horizontal line
                    S32 x = GetSBits(nBits);
                    xLast += x;

                    printf("\thlineto: (%d,%d).\n", xLast, yLast);
                }
            }
        }
        else
        {
            // Handle a curve
            U16 nBits = (U16) GetBits(4) + 2;   // nBits is biased by 2

            // Get the control
            S32 cx = GetSBits(nBits);
            S32 cy = GetSBits(nBits);
            xLast += cx;
            yLast += cy;

            printf("\tcurveto: (%d,%d)", xLast, yLast);

            // Get the anchor
            S32 ax = GetSBits(nBits);
            S32 ay = GetSBits(nBits);
            xLast += ax;
            yLast += ay;

            printf("(%d,%d)\n", xLast, yLast);
        }

        return false;
    }
}

void CInputScript::ParseShapeStyle(char *str)
{
    U16 i = 0;

    // Get the number of fills.
    U16 nFills = GetByte();

    // Do we have a larger number?
    if (nFills == 255)
    {
        // Get the larger number.
        nFills = GetWord();
    }

    printf("\tNumber of fill styles \t%u\n", nFills);

    // Get each of the fill style.
    for (i = 1; i <= nFills; i++)
    {
        U16 fillStyle = GetByte();

        printf("\tFill Style #%d is %d\n", i, fillStyle);

        if (fillStyle & fillGradient)
        {
            // Get the gradient matrix.
            MATRIX mat;
            GetMatrix(&mat);
            PrintMatrix(mat, str);

            // Get the number of colors.
            U16 nColors = (U16) GetByte();

            // Get each of the colors.
            for (U16 j = 0; j < nColors; j++)
            {
                GetByte();
                GetColor();
            }

            printf("%s\tGradient Fill with %u colors\n", str, nColors);
        }
        else if (fillStyle & fillBits)
        {
            // Get the bitmap matrix.
            printf("%s\tBitmap Fill\n", str);
            MATRIX mat;
            GetMatrix(&mat);
        }
        else
        {
            // A solid color
            U32 color = GetColor();
            printf("%s\tSolid Color Fill RGB_HEX %06x\n", str, color);
        }
    }

    // Get the number of lines.
    U16 nLines = GetByte();

    // Do we have a larger number?
    if (nLines == 255)
    {
        // Get the larger number.
        nLines = GetWord();
    }

    printf("\tNumber of line styles \t%u\n", nLines);

    // Get each of the line styles.
    for (i = 1; i <= nLines; i++)
    {
        U16 width = GetWord();
        U32 color = GetColor();
    
        printf("\tLine style %-5u width %g color RGB_HEX %06x\n", i, (double)width/20.0, color);
    }
}

void CInputScript::ParseDefineShape(char *str)
{
    U32 tagid = (U32) GetWord();
    printf("%stagDefineShape \ttagid %-5u\n", str, tagid);

    // Get the bounding rectangle
    SRECT rect;
    GetRect(&rect);

    // ShapeWithStyle
    BOOL atEnd = false;

    ParseShapeStyle(str);

    InitBits();     // Bug!  this was not in the original swfparse.cpp
                    // Required to reset bit counters and read byte aligned.

    m_nFillBits = (U16) GetBits(4);
    m_nLineBits = (U16) GetBits(4);

    //printf("m_nFillBits:%d  m_nLineBits:%d\n", m_nFillBits, m_nLineBits);

    int xLast = 0;
    int yLast = 0;

    while (!atEnd)
        atEnd = ParseShapeRecord(str, xLast, yLast);
}
