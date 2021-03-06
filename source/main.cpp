#include <3ds.h>
#include <sf2d.h>
#include <sftd.h>
#include <vector>
#include <cmath>
#include <sstream>
#include "ViewWindow.h"
#include "BmpFont.h"
#include "RpnInstruction.h"
#include "TableLayout.h"
#include "ControlGrid.h"
#include "Button.h"
#include "TextDisplay.h"
#include "NumpadController.h"
#include "Slider.h"

constexpr int plotCount = 4;

std::vector<RpnInstruction> equations[plotCount];
float exprX;
BmpFont mainFont, btnFont;
TextDisplay *equDisp;
Control *btnBackspace;
NumpadController numpad;
std::vector<ControlGridBase*> controlGrids;
int cgridIndex = 0;
int plotIndex = 0;
int keys = 0, down = 0;
bool altMode = false;
ViewWindow view(-5.0f, 5.0f, -3.0f, 3.0f);

const int plotColors[] = { RGBA8(0x00, 0x00, 0xC0, 0xFF), RGBA8(0x00, 0x80, 0x00, 0xFF), RGBA8(0xD5, 0x00, 0xDD, 0xFF), RGBA8(0xD2, 0x94, 0x00, 0xFF) };

void UpdateEquationDisplay();
void SetUpMainControlGrid(ControlGrid<5, 7> &cgrid);
void SetUpVarsControlGrid(ControlGrid<5, 7> &cgrid);

void drawAxes(const ViewWindow &view, u32 color, float originX = 0.0f, float originY = 0.0f, bool hideHorizontal = false)
{
	Point<int> center = view.GetScreenCoords(originX, originY);
	if (0 <= center.x && center.x < 400) {
		sf2d_draw_rectangle(center.x, 0, 1, 240, color);
	}
	if (!hideHorizontal && 0 <= center.y && center.y < 240) {
		sf2d_draw_rectangle(0, center.y, 399, 1, color);
	}
}

void drawGraph(const std::vector<RpnInstruction> &equation, const ViewWindow &view, u32 color, bool showErrors = true)
{
	Point<int> lastPoint;
	bool ignoreLastPoint = true;
	
	for (int x=0; x<400; x++) {
		Point<int> pt;
		exprX = Interpolate((float)x, 0.0f, 399.0f, view.xmin, view.xmax);
		float y;
		RpnInstruction::Status status = ExecuteRpn(equation, y);
		pt = view.GetScreenCoords(exprX, y);
		
		if (status == RpnInstruction::S_OK && !ignoreLastPoint) {
			sf2d_draw_line(lastPoint.x, lastPoint.y, pt.x, pt.y, 2.0f, color);
		} else if (status == RpnInstruction::S_OVERFLOW) {
			if (showErrors) {
                mainFont.drawStr("Error: Stack overflow", 4, 4, color);
			}
			break;
		} else if (status == RpnInstruction::S_UNDERFLOW) {
			if (showErrors) {
                mainFont.drawStr("Error: Stack underflow", 4, 4, color);
			}
			break;
		} else {
			ignoreLastPoint = (status == RpnInstruction::S_UNDEFINED);
		}
		
		lastPoint = pt;
	}
}

void moveCursor(float &cursorX, float &cursorY, float dx, float dy)
{
	cursorX += dx;
	if (cursorX < 0.0f)
		cursorX = 0.0f;
	else if (cursorX > 399.0f)
		cursorX = 399.0f;
	
	cursorY += dy;
	if (cursorY < 0.0f)
		cursorY = 0.0f;
	else if (cursorY > 239.0f)
		cursorY = 239.0f;
}

void addInstruction(const RpnInstruction &inst)
{
	if (numpad.EntryInProgress()) {
		numpad.Reset();
	}
	equations[plotIndex].push_back(inst);
	UpdateEquationDisplay();
}

int main(int argc, char *argv[])
{
	float cursorX = 200.0f, cursorY = 120.0f;
	float traceUnit = 0;
	bool traceUndefined = false;
	
	equations[0].push_back(RpnInstruction(&exprX, "x"));
	equations[0].push_back(RpnInstruction(std::sin, "sin"));
	
	ControlGrid<5, 7> cgridMain(45, 48);
	cgridMain.SetDrawOffset(2, 0);
	SetUpMainControlGrid(cgridMain);
	controlGrids.push_back(&cgridMain);
	
	ControlGrid<5, 7> cgridVars(45, 48);
	cgridVars.SetDrawOffset(2, 0);
	SetUpVarsControlGrid(cgridVars);
	controlGrids.push_back(&cgridVars);
	
	sf2d_init();
	sf2d_set_clear_color(RGBA8(0xE0, 0xE0, 0xE0, 0xFF));
	
	romfsInit();
	mainFont.load("romfs:/mainfont.bff");
    btnFont.load("romfs:/buttons.bff");
	
	while (aptMainLoop()) {
		hidScanInput();
		keys = hidKeysHeld();
		down = hidKeysDown();
		
		touchPosition touch;
		if (keys & KEY_TOUCH) {
			hidTouchRead(&touch);
		}
		
		if (keys & KEY_START) {
			break;
		}
		
		if ((keys & KEY_L) && !(keys & KEY_TOUCH)) {
			view.ZoomOut(1.02f);
		}
		
		if ((keys & KEY_R) && !(keys & KEY_TOUCH)) {
			view.ZoomIn(1.02f);
		}
		
		if ((down & KEY_SELECT) && !(keys & KEY_TOUCH)) {
			altMode = !altMode;
		}
		
		if (down & (KEY_DUP | KEY_DDOWN)) {
			if (keys & KEY_X) {
				if (down & KEY_DUP) moveCursor(cursorX, cursorY, 0.0f, -1.0f);
				if (down & KEY_DDOWN) moveCursor(cursorX, cursorY, 0.0f, 1.0f);
			} else {
				numpad.Reset();
				
				if (down & KEY_DUP) --plotIndex;
				if (down & KEY_DDOWN) ++plotIndex;
				
				if (plotIndex < 0)
					plotIndex = plotCount - 1;
				else if (plotIndex >= plotCount)
					plotIndex = 0;
				
				UpdateEquationDisplay();
			}
		}
		
		if (down & (KEY_DLEFT | KEY_DRIGHT)) {
			if (keys & (KEY_X | KEY_Y)) {
				if (down & KEY_DLEFT) moveCursor(cursorX, cursorY, -1.0f, 0.0f);
				if (down & KEY_DRIGHT) moveCursor(cursorX, cursorY, 1.0f, 0.0f);
			} else if (!(keys & KEY_TOUCH)) {
				if (down & KEY_DLEFT) --cgridIndex;
				if (down & KEY_DRIGHT) ++cgridIndex;
			
				int count = controlGrids.size();
				if (cgridIndex < 0)
					cgridIndex = count - 1;
				else if (cgridIndex >= count) {
					cgridIndex = 0;
				}
			}
		}
		
		circlePosition circle;
		hidCircleRead(&circle);
		
		if (circle.dx * circle.dx + circle.dy * circle.dy > 20*20) {
			if (keys & (KEY_X | KEY_Y)) {
				moveCursor(cursorX, cursorY, 0.05f * circle.dx, -0.05f * circle.dy);
			} else {
				float rangeX = view.xmax - view.xmin;
				float rangeY = view.ymax - view.ymin;
				view.Pan(0.0002f * rangeX * circle.dx, 0.0002f * rangeY * circle.dy);
			}
		}
		
		sf2d_start_frame(GFX_TOP, GFX_LEFT);
		sf2d_draw_rectangle(0, 0, 400, 240, RGBA8(0xFF, 0xFF, 0xFF, 0xFF));
		drawAxes(view, RGBA8(0x80, 0xFF, 0xFF, 0xFF));
		
		for (int i=0; i<plotCount; i++) {
			drawGraph(equations[i], view, plotColors[i], i == plotIndex);
		}
		
		if (keys & (KEY_X | KEY_Y)) {
			Point<float> cursor = view.GetGraphCoords(cursorX, cursorY);
			if (keys & KEY_Y) {
				if (keys & KEY_B) {
					traceUnit = std::pow(10.0f, std::ceil(std::log10((view.xmax - view.xmin) / 400)));
					cursor.x = std::round(cursor.x / traceUnit) * traceUnit;
				}
				exprX = cursor.x;
				RpnInstruction::Status status = ExecuteRpn(equations[plotIndex], cursor.y);
				traceUndefined = (status != RpnInstruction::S_OK);
			} else {
				traceUndefined = false;
			}
			u32 color = (keys & KEY_Y) ? RGBA8(0xFF, 0x00, 0x00, 0xFF) : RGBA8(0x00, 0xC0, 0x00, 0xFF);
			drawAxes(view, color, cursor.x, cursor.y, traceUndefined);
            mainFont.drawStr(ssprintf("X = %.5f", cursor.x), 2, 0, color);
			if (!traceUndefined)
                mainFont.drawStr(ssprintf("Y = %.5f", cursor.y), 2, 22, color);
		}
        if (altMode) btnFont.align(ALIGN_LEFT).drawStr("ALT", 2, 225, RGBA8(0x48, 0x67, 0x4E, 0xFF));
		sf2d_end_frame();
		
		sf2d_start_frame(GFX_BOTTOM, GFX_LEFT);
		controlGrids[cgridIndex]->ScreenTouchStatus(keys & KEY_TOUCH, touch.px, touch.py);
		controlGrids[cgridIndex]->Draw();
		sf2d_end_frame();
		
		sf2d_swapbuffers();
	}
	
	romfsExit();
	sf2d_fini();
	
	return 0;
}

void UpdateEquationDisplay()
{
	std::ostringstream ss;
	auto count = equations[plotIndex].size();
	if (numpad.EntryInProgress()) --count;
	
	for (decltype(count) i=0; i<count; i++) {
		ss << equations[plotIndex][i] << ' ';
	}
	
	if (numpad.EntryInProgress()) {
		std::string entry;
		numpad.GetEntryString(entry);
		ss << entry;
	}
	
	equDisp->SetText(ss.str());
	equDisp->SetTextColor(plotColors[plotIndex]);
}

void SetUpMainControlGrid(ControlGrid<5, 7> &cgrid)
{
	equDisp = new TextDisplay();
	cgrid.cells[0][0].content = equDisp;
	UpdateEquationDisplay();
	
	const char *btnText[5][7] = {
		{nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "\xAB"},
		{"7",     "8",     "9",     "\xF7",  "^",     "mod",   "abs"},
		{"4",     "5",     "6",     "\xD7",  "sqrt",  "exp",   "ln"},
		{"1",     "2",     "3",     "-",     "sin",   "cos",   "tan"},
		{"0",     ".",     "+/-",   "+",     "x",     "clear", nullptr}
	};
	
	const char *btnTextAlt[5][7] = {
		{nullptr,     nullptr,     nullptr,     nullptr,     nullptr,     nullptr,     nullptr},
		{nullptr,     nullptr,     nullptr,     nullptr,     nullptr,     nullptr,     nullptr},
		{nullptr,     nullptr,     nullptr,     nullptr,     nullptr,     nullptr,     "log"},
		{nullptr,     nullptr,     nullptr,     nullptr,     "asin",      "acos",      "atan"},
		{nullptr,     nullptr,     nullptr,     nullptr,     "dup",       "def. view", nullptr}
	};
	
	constexpr Button::ColorPreset C_BLUE = Button::ColorPreset::C_BLUE;
	constexpr Button::ColorPreset C_GREEN = Button::ColorPreset::C_GREEN;
	constexpr Button::ColorPreset C_PINK = Button::ColorPreset::C_PINK;
	constexpr Button::ColorPreset C_ORANGE = Button::ColorPreset::C_ORANGE;
	
	const Button::ColorPreset btnColors[5][7] = {
		{C_BLUE,   C_BLUE,   C_BLUE,   C_BLUE,   C_BLUE,   C_BLUE,   C_ORANGE},
		{C_BLUE,   C_BLUE,   C_BLUE,   C_GREEN,  C_GREEN,  C_GREEN,  C_PINK},
		{C_BLUE,   C_BLUE,   C_BLUE,   C_GREEN,  C_PINK,   C_PINK,   C_PINK},
		{C_BLUE,   C_BLUE,   C_BLUE,   C_GREEN,  C_PINK,   C_PINK,   C_PINK},
		{C_BLUE,   C_BLUE,   C_BLUE,   C_GREEN,  C_BLUE,   C_ORANGE, C_ORANGE}
	};
	
	const RpnInstruction btnInstructions[5][7] = {
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_DIVIDE, RpnInstruction::OP_POWER, RpnInstruction::OP_MODULO, RpnInstruction(std::abs, "abs") },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_MULTIPLY, RpnInstruction(std::sqrt, "sqrt", ~RpnInstruction::D_NEGATIVE), RpnInstruction(std::exp, "exp"), RpnInstruction(std::log, "ln", RpnInstruction::D_POSITIVE) },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_SUBTRACT, RpnInstruction(std::sin, "sin"), RpnInstruction(std::cos, "cos"), RpnInstruction(std::tan, "tan") },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_ADD, RpnInstruction(&exprX, "x"), RpnInstruction::OP_NULL, RpnInstruction::OP_NULL }
	};
	
	const RpnInstruction btnInstructionsAlt[5][7] = {
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction(std::log10, "log", RpnInstruction::D_POSITIVE) },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction(std::asin, "asin"), RpnInstruction(std::acos, "acos"), RpnInstruction(std::atan, "atan") },
		{RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL, RpnInstruction::OP_DUP, RpnInstruction::OP_NULL, RpnInstruction::OP_NULL }
	};
	
	const char *numpadKeys[] = { "789", "456", "123", "0.-" };
	
	for (int r=0; r<5; r++) {
		for (int c=0; c<7; c++) {
			if (btnText[r][c] != nullptr) {
				Button *btn = new Button(btnText[r][c], btnColors[r][c]);
				if (btnTextAlt[r][c] != nullptr) btn->SetText(btnTextAlt[r][c], true);
				RpnInstruction::Opcode opcode = btnInstructions[r][c].GetOpcode();
				
				if (r == 4 && c == 5) {
					//clear button
					btn->SetAction([](Button&) {
						if (altMode) {
							view = ViewWindow(-5.0f, 5.0f, -3.0f, 3.0f);
						} else {
							equations[plotIndex].clear();
							numpad.Reset();
							UpdateEquationDisplay();
						}
					});
				} else if ((r > 0 && c < 3) || (r == 0 && c == 6)) {
					//one of the numpad keys, or backspace
					char key;
					if (r == 0) { //&& c == 6, always the case if r == 0
						key = '\b';
					} else {
						key = numpadKeys[r-1][c];
					}
					btn->SetAction([key](Button&) {
						if (key == '\b' && !numpad.EntryInProgress()) {
							if (equations[plotIndex].size() > 0) {
								equations[plotIndex].pop_back();
							}
						} else {
							const RpnInstruction *lastInst = nullptr;
							if (equations[plotIndex].size() > 0) {
								lastInst = &equations[plotIndex].back();
							}
							NumpadController::Reply reply = numpad.KeyPressed(key, lastInst);
							if (reply.replaceLast) {
								equations[plotIndex].back() = reply.inst;
							} else {
								equations[plotIndex].push_back(reply.inst);
							}
						}
						UpdateEquationDisplay();
					});
				} else if (opcode != RpnInstruction::OP_NULL) {
					//one of the buttons that adds an RPN instruction
					RpnInstruction inst = btnInstructions[r][c];
					RpnInstruction instAlt = btnInstructionsAlt[r][c];
					if (instAlt.GetOpcode() == RpnInstruction::OP_NULL) instAlt = inst;
					btn->SetAction([inst, instAlt](Button&) {
						addInstruction(altMode ? instAlt : inst);
					});
				}
				
				cgrid.cells[r][c].content = btn;
			}
		}
	}
	
	btnBackspace = cgrid.cells[0][6].content;
	
	cgrid.cells[0][0].colSpan = 6;
	cgrid.cells[4][5].colSpan = 2;
}

void SetUpVarsControlGrid(ControlGrid<5, 7> &cgrid)
{
	cgrid.cells[0][0].content = equDisp;
	cgrid.cells[0][0].colSpan = 6;
	cgrid.cells[0][6].content = btnBackspace;
	
	for (int i=0; i<4; i++) {
		Slider *slider = new Slider();
		slider->value = 0.5f;
		cgrid.cells[i+1][1] = slider;
		cgrid.cells[i+1][1].colSpan = 5;
		
		char varName[2] = {(char)('a' + i), '\0'};
		Button *btn = new Button(varName, Button::C_BLUE);
		btn->SetText(">|", true);
		btn->SetAction([slider, varName](Button&) {
			if (altMode) {
				slider->SetMinimum(slider->value);
			} else {
				addInstruction(RpnInstruction(&slider->value, varName));
			}
		});
		cgrid.cells[i+1][0].content = btn;
		
		btn = new Button("0-1");
		btn->SetText("|<", true);
		btn->SetColors(Button::C_ORANGE, Button::C_BLUE);
		btn->SetAction([slider](Button&) {
			if (altMode) {
				slider->SetMaximum(slider->value);
			} else {
				slider->SetRange(0.0f, 1.0f);
				slider->value = 0.5f;
			}
		});
		cgrid.cells[i+1][6].content = btn;
	}
}
