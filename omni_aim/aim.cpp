#include "aim.h"


void Aim::move_cursor(long dx, long dy) {

	POINT current_pos;
	bool ret;
	ret = GetCursorPos(&current_pos);
	ret =  SetCursorPos(current_pos.x + dx, current_pos.y + dy);
}