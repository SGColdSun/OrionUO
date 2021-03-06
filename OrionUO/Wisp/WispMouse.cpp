//----------------------------------------------------------------------------------
#include "WispMouse.h"
#include "WispWindow.h"

namespace WISP_MOUSE
{
//forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable: 4800)

CMouse *g_WispMouse = NULL;
//----------------------------------------------------------------------------------
CMouse::CMouse()
: m_Position(), m_LeftDropPosition(), m_RightDropPosition(), m_MidDropPosition(),
m_LeftButtonPressed(false), m_RightButtonPressed(false), m_MidButtonPressed(false),
m_Dragging(false), m_DoubleClickDelay(350), m_CancelDoubleClick(false),
m_LastLeftButtonClickTimer(0), m_LastRightButtonClickTimer(0), m_LastMidButtonClickTimer(0),
m_RealPosition()
{
	g_WispMouse = this;
}
//----------------------------------------------------------------------------------
CMouse::~CMouse()
{
	g_WispMouse = NULL;
}
//----------------------------------------------------------------------------------
void CMouse::Update()
{
	POINT pos;
	GetCursorPos(&pos);

	if (WISP_WINDOW::g_WispWindow != NULL)
		ScreenToClient(WISP_WINDOW::g_WispWindow->Handle, &pos);

	m_Dragging = (m_LeftButtonPressed || m_RightButtonPressed || m_MidButtonPressed);

	m_Position.X = pos.x;
	m_Position.Y = pos.y;

	m_RealPosition = m_Position;
}
//----------------------------------------------------------------------------------
void CMouse::Update(WPARAM wParam, LPARAM lParam)
{
	m_LeftButtonPressed = (bool)(wParam & MK_LBUTTON);
	m_RightButtonPressed = (bool)(wParam & MK_RBUTTON);
	m_MidButtonPressed = (bool)(wParam & MK_MBUTTON);

	m_Dragging = (m_LeftButtonPressed || m_RightButtonPressed || m_MidButtonPressed);

	POINT pos;
	GetCursorPos(&pos);

	if (WISP_WINDOW::g_WispWindow != NULL)
		ScreenToClient(WISP_WINDOW::g_WispWindow->Handle, &pos);

	m_Position.X = pos.x;
	m_Position.Y = pos.y;

	//m_Position.X = LOWORD(lParam);
	//m_Position.Y = HIWORD(lParam);

	m_RealPosition = m_Position;
}
//----------------------------------------------------------------------------------
void CMouse::ReleaseCapture()
{
	if (!(m_LeftButtonPressed || m_RightButtonPressed || m_MidButtonPressed))
		::ReleaseCapture();
}
//----------------------------------------------------------------------------------
WISP_GEOMETRY::CPoint2Di CMouse::LeftDroppedOffset()
{
	WISP_GEOMETRY::CPoint2Di position;

	if (m_LeftButtonPressed)
	{
		position.X = m_RealPosition.X - m_LeftDropPosition.X;
		position.Y = m_RealPosition.Y - m_LeftDropPosition.Y;
	}

	return position;
}
//----------------------------------------------------------------------------------
WISP_GEOMETRY::CPoint2Di CMouse::RightDroppedOffset()
{
	WISP_GEOMETRY::CPoint2Di position;

	if (m_RightButtonPressed)
	{
		position.X = m_RealPosition.X - m_RightDropPosition.X;
		position.Y = m_RealPosition.Y - m_RightDropPosition.Y;
	}

	return position;
}
//----------------------------------------------------------------------------------
WISP_GEOMETRY::CPoint2Di CMouse::MidDroppedOffset()
{
	WISP_GEOMETRY::CPoint2Di position;

	if (m_MidButtonPressed)
	{
		position.X = m_RealPosition.X - m_MidDropPosition.X;
		position.Y = m_RealPosition.Y - m_MidDropPosition.Y;
	}

	return position;
}
//----------------------------------------------------------------------------------
}; //namespace
//----------------------------------------------------------------------------------
