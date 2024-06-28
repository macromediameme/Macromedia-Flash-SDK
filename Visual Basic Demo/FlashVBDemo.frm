VERSION 5.00
Object = "{D27CDB6B-AE6D-11CF-96B8-444553540000}#1.0#0"; "swflash.ocx"
Begin VB.Form FlashVBDemo 
   Caption         =   "FlashVBDemo"
   ClientHeight    =   7164
   ClientLeft      =   48
   ClientTop       =   324
   ClientWidth     =   5436
   LinkTopic       =   "Form1"
   ScaleHeight     =   7164
   ScaleWidth      =   5436
   StartUpPosition =   3  'Windows Default
   Begin VB.CommandButton ZoomIn 
      Caption         =   "Zoom In"
      Height          =   372
      Left            =   360
      TabIndex        =   7
      Top             =   4200
      Width           =   972
   End
   Begin VB.Frame View 
      Caption         =   "View"
      Height          =   852
      Left            =   240
      TabIndex        =   6
      Top             =   3840
      Width           =   4932
      Begin VB.CommandButton ZoomOut 
         Caption         =   "Zoom Out"
         Height          =   372
         Left            =   1320
         TabIndex        =   10
         Top             =   360
         Width           =   972
      End
      Begin VB.CommandButton High 
         Caption         =   "High"
         Height          =   372
         Left            =   2640
         TabIndex        =   9
         Top             =   360
         Width           =   972
      End
      Begin VB.CommandButton Low 
         Caption         =   "Low"
         Height          =   372
         Left            =   3840
         TabIndex        =   8
         Top             =   360
         Width           =   972
      End
   End
   Begin VB.CommandButton Back 
      Caption         =   "Back"
      Height          =   372
      Left            =   4080
      TabIndex        =   4
      Top             =   360
      Width           =   972
   End
   Begin VB.CommandButton Forward 
      Caption         =   "Forward"
      Height          =   372
      Left            =   2880
      TabIndex        =   3
      Top             =   360
      Width           =   972
   End
   Begin VB.CommandButton Play 
      Caption         =   "Play"
      Height          =   372
      Left            =   360
      TabIndex        =   2
      Top             =   360
      Width           =   972
   End
   Begin VB.CommandButton Rewind 
      Caption         =   "Rewind"
      Height          =   372
      Left            =   1560
      TabIndex        =   1
      Top             =   360
      Width           =   972
   End
   Begin ShockwaveFlashObjectsCtl.ShockwaveFlash ShockwaveFlash1 
      Height          =   2772
      Left            =   240
      TabIndex        =   0
      Top             =   960
      Width           =   4932
      Movie           =   "f:\vbasic\shockrave.swf"
      Src             =   "f:\vbasic\shockrave.swf"
      WMode           =   "Window"
      Play            =   -1  'True
      Loop            =   -1  'True
      Quality         =   "AutoHigh"
      SAlign          =   ""
      Menu            =   -1  'True
      Base            =   ""
      Scale           =   "ShowAll"
      DeviceFont      =   0   'False
      EmbedMovie      =   -1  'True
      BGColor         =   ""
   End
   Begin VB.Frame Frame1 
      Caption         =   "Control"
      Height          =   852
      Left            =   240
      TabIndex        =   5
      Top             =   0
      Width           =   4932
   End
   Begin ShockwaveFlashObjectsCtl.ShockwaveFlash ShockwaveFlash2 
      Height          =   2172
      Left            =   240
      TabIndex        =   11
      Top             =   4800
      Width           =   4932
      Movie           =   "f:\vbasic\uiexample.swf"
      Src             =   "f:\vbasic\uiexample.swf"
      WMode           =   "Transparent"
      Play            =   0   'False
      Loop            =   -1  'True
      Quality         =   "AutoHigh"
      SAlign          =   ""
      Menu            =   -1  'True
      Base            =   ""
      Scale           =   "ShowAll"
      DeviceFont      =   0   'False
      EmbedMovie      =   -1  'True
      BGColor         =   ""
   End
End
Attribute VB_Name = "FlashVBDemo"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Private Sub High_Click()
    ShockwaveFlash1.Quality2 = "high"
End Sub

Private Sub Low_Click()
    ShockwaveFlash1.Quality2 = "low"
End Sub

Private Sub Rewind_Click()
    ShockwaveFlash1.GotoFrame (0)
End Sub

Private Sub Play_Click()
    ShockwaveFlash1.Play
End Sub

Private Sub Forward_Click()
    ShockwaveFlash1.Forward
End Sub

Private Sub Back_Click()
    ShockwaveFlash1.Back
End Sub


Private Sub ShockwaveFlash2_FSCommand(ByVal command As String, ByVal args As String)
    If (command = "Zoom") Then
        ShockwaveFlash1.Zoom (args)
    Else
       MsgBox (args)
    End If
        
End Sub


Private Sub ZoomIn_Click()
    ShockwaveFlash1.Zoom (50)
End Sub

Private Sub ZoomOut_Click()
    ShockwaveFlash1.Zoom (200)
End Sub
