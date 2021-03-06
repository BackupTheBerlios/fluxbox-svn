//This file is generated from nls/C
#ifndef BLACKBOX_NLS_HH
#define BLACKBOX_NLS_HH
namespace FBNLS {
enum {
	BaseDisplaySet = 0x1,
	BaseDisplayXError = 0x1,
	BaseDisplaySignalCaught = 0x2,
	BaseDisplayShuttingDown = 0x3,
	BaseDisplayAborting = 0x4,
	BaseDisplayXConnectFail = 0x5,
	BaseDisplayCloseOnExecFail = 0x6,
	BaseDisplayBadWindowRemove = 0x7,

	BasemenuSet = 0x2,
	BasemenuBlackboxMenu = 0x1,

	ConfigmenuSet = 0x3,
	ConfigmenuConfigOptions = 0x1,
	ConfigmenuFocusModel = 0x2,
	ConfigmenuWindowPlacement = 0x3,
	ConfigmenuImageDithering = 0x4,
	ConfigmenuOpaqueMove = 0x5,
	ConfigmenuFullMax = 0x6,
	ConfigmenuFocusNew = 0x7,
	ConfigmenuFocusLast = 0x8,
	ConfigmenuClickToFocus = 0x9,
	ConfigmenuSloppyFocus = 0xa,
	ConfigmenuAutoRaise = 0xb,
	ConfigmenuSmartRows = 0xc,
	ConfigmenuSmartCols = 0xd,
	ConfigmenuCascade = 0xe,
	ConfigmenuLeftRight = 0xf,
	ConfigmenuRightLeft = 0x10,
	ConfigmenuTopBottom = 0x11,
	ConfigmenuBottomTop = 0x12,
	ConfigmenuTabs = 0x13,
	ConfigmenuIcons = 0x14,
	ConfigmenuTabPlacement = 0x15,
	ConfigmenuTabRotateVertical = 0x16,
	ConfigmenuSemiSloppyFocus = 0x17,
	ConfigmenuMaxOverSlit = 0x18,
	ConfigmenuSloppyWindowGrouping = 0x19,
	ConfigmenuWorkspaceWarping = 0x1a,
	ConfigmenuDesktopWheeling = 0x1b,
	ConfigmenuAntiAlias = 0x1c,
	ConfigmenuDecorateTransient = 0x1d,

	IconSet = 0x4,
	IconIcons = 0x1,

	ImageSet = 0x5,
	ImageErrorCreatingSolidPixmap = 0x1,
	ImageErrorCreatingXImage = 0x2,
	ImageUnsupVisual = 0x3,
	ImageErrorCreatingPixmap = 0x4,
	ImageInvalidColormapSize = 0x5,
	ImageErrorAllocatingColormap = 0x6,
	ImageColorAllocFail = 0x7,
	ImagePixmapRelease = 0x8,
	ImagePixmapCacheLarge = 0x9,
	ImageColorParseError = 0xa,
	ImageColorAllocError = 0xb,

	ScreenSet = 0x6,
	ScreenAnotherWMRunning = 0x1,
	ScreenManagingScreen = 0x2,
	ScreenFontLoadFail = 0x3,
	ScreenDefaultFontLoadFail = 0x4,
	ScreenEmptyMenuFile = 0x5,
	Screenxterm = 0x6,
	ScreenRestart = 0x7,
	ScreenExit = 0x8,
	ScreenEXECError = 0x9,
	ScreenEXITError = 0xa,
	ScreenSTYLEError = 0xb,
	ScreenCONFIGError = 0xc,
	ScreenINCLUDEError = 0xd,
	ScreenINCLUDEErrorReg = 0xe,
	ScreenSUBMENUError = 0xf,
	ScreenRESTARTError = 0x10,
	ScreenRECONFIGError = 0x11,
	ScreenSTYLESDIRError = 0x12,
	ScreenSTYLESDIRErrorNotDir = 0x13,
	ScreenSTYLESDIRErrorNoExist = 0x14,
	ScreenWORKSPACESError = 0x15,
	ScreenPositionLength = 0x16,
	ScreenPositionFormat = 0x17,
	ScreenGeometryFormat = 0x18,
	ScreenGeometryLength = 0x19,

	SlitSet = 0x7,
	SlitSlitTitle = 0x1,
	SlitSlitDirection = 0x2,
	SlitSlitPlacement = 0x3,

	ToolbarSet = 0x8,
	ToolbarNoStrftimeLength = 0x1,
	ToolbarNoStrftimeDateFormat = 0x2,
	ToolbarNoStrftimeDateFormatEu = 0x3,
	ToolbarNoStrftimeTimeFormat24 = 0x4,
	ToolbarNoStrftimeTimeFormat12 = 0x5,
	ToolbarNoStrftimeTimeFormatP = 0x6,
	ToolbarNoStrftimeTimeFormatA = 0x7,
	ToolbarToolbarTitle = 0x8,
	ToolbarEditWkspcName = 0x9,
	ToolbarToolbarPlacement = 0xa,

	WindowSet = 0x9,
	WindowCreating = 0x1,
	WindowXGetWindowAttributesFail = 0x2,
	WindowCannotFindScreen = 0x3,
	WindowUnnamed = 0x4,
	WindowMapRequest = 0x5,
	WindowUnmapNotify = 0x6,
	WindowUnmapNotifyReparent = 0x7,

	WindowmenuSet = 0xa,
	WindowmenuSendTo = 0x1,
	WindowmenuSendGroupTo = 0x2,
	WindowmenuShade = 0x3,
	WindowmenuIconify = 0x4,
	WindowmenuMaximize = 0x5,
	WindowmenuRaise = 0x6,
	WindowmenuLower = 0x7,
	WindowmenuStick = 0x8,
	WindowmenuKillClient = 0x9,
	WindowmenuClose = 0xa,
	WindowmenuTab = 0xb,

	WorkspaceSet = 0xb,
	WorkspaceDefaultNameFormat = 0x1,

	WorkspacemenuSet = 0xc,
	WorkspacemenuWorkspacesTitle = 0x1,
	WorkspacemenuNewWorkspace = 0x2,
	WorkspacemenuRemoveLast = 0x3,

	blackboxSet = 0xd,
	blackboxNoManagableScreens = 0x1,
	blackboxMapRequest = 0x2,

	CommonSet = 0xe,
	CommonYes = 0x1,
	CommonNo = 0x2,
	CommonDirectionTitle = 0x3,
	CommonDirectionHoriz = 0x4,
	CommonDirectionVert = 0x5,
	CommonAlwaysOnTop = 0x6,
	CommonPlacementTitle = 0x7,
	CommonPlacementTopLeft = 0x8,
	CommonPlacementCenterLeft = 0x9,
	CommonPlacementBottomLeft = 0xa,
	CommonPlacementTopCenter = 0xb,
	CommonPlacementBottomCenter = 0xc,
	CommonPlacementTopRight = 0xd,
	CommonPlacementCenterRight = 0xe,
	CommonPlacementBottomRight = 0xf,
	CommonPlacementLeftTop = 0x10,
	CommonPlacementLeftCenter = 0x11,
	CommonPlacementLeftBottom = 0x12,
	CommonPlacementRightTop = 0x13,
	CommonPlacementRightCenter = 0x14,
	CommonPlacementRightBottom = 0x15,
	CommonPlacementTopRelative = 0x16,
	CommonPlacementBottomRelative = 0x17,
	CommonPlacementLeftRelative = 0x18,
	CommonPlacementRightRelative = 0x19,
	CommonAutoHide = 0x1a,

	mainSet = 0xf,
	mainRCRequiresArg = 0x1,
	mainDISPLAYRequiresArg = 0x2,
	mainWarnDisplaySet = 0x3,
	mainUsage = 0x4,
	mainCompileOptions = 0x5,

	bsetrootSet = 0x10,
	bsetrootMustSpecify = 0x1,
	bsetrootUsage = 0x2,
	dummy_not_used = 0 //just for the ending
}; //end enum
}; //end namespace
#endif //BLACKBOX_NLS_HH
