#ifndef OM_LOG_ANSI_HPP
#define OM_LOG_ANSI_HPP

#define OMLogAnsiReset toAnsi(0)
#define OMLogAnsiBold toAnsi(1)
#define OMLogAnsiFaint toAnsi(2)
#define OMLogAnsiItalic toAnsi(3)
#define OMLogAnsiUnderline toAnsi(4)
#define OMLogAnsiRev toAnsi(7)
#define OMLogAnsiStroke toAnsi(9)

#define OMLogAnsiBlack toAnsi(30)
#define OMLogAnsiBlackLight toAnsi(90)
#define OMLogAnsiRed toAnsi(31)
#define OMLogAnsiRedLight toAnsi(91)
#define OMLogAnsiGreen toAnsi(32)
#define OMLogAnsiGreenLight toAnsi(92)
#define OMLogAnsiYellow toAnsi(33)
#define OMLogAnsiYellowLight toAnsi(93)
#define OMLogAnsiBlue toAnsi(34)
#define OMLogAnsiBlueLight toAnsi(94)
#define OMLogAnsiMagenta toAnsi(35)
#define OMLogAnsiMagentaLight toAnsi(95)
#define OMLogAnsiCyan toAnsi(36)
#define OMLogAnsiCyanLight toAnsi(96)
#define OMLogAnsiWhite toAnsi(37)
#define OMLogAnsiWhiteLight toAnsi(97)

#include <string>

namespace openminecraft::log::ansi
{
std::string toAnsi(int code);
}

#endif