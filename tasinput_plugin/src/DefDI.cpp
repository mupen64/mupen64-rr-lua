/*
Copyright (C) 2001 Deflection

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/*
	Modified for TAS by Nitsuja
*/


#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <stdio.h>
#include <math.h>
#include <vector>
#include "Controller.h"
#include "DI.h"
#include "DefDI.h"
#include "Config.h"
#include "resource.h"

#ifdef DEBUG
#define PLUGIN_NAME "TAS Input debug"
#else
#define PLUGIN_NAME "TAS Input"
#endif

#define PI 3.14159265358979f
#define BUFFER_CHUNK 128

#define aCombo ComboList.at(activeCombo) //so it's a bit cleaner

HINSTANCE g_hInstance;

GUID Guids[MAX_DEVICES];
DEFCONTROLLER Controller[NUMBER_OF_CONTROLS];
CONTROL *ControlDef[NUMBER_OF_CONTROLS];
//LRESULT CALLBACK StatusDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusDlgProc0 (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusDlgProc1 (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusDlgProc2 (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StatusDlgProc3 (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI StatusDlgThreadProc (LPVOID lpParameter);
bool romIsOpen = false;
bool erase = true;

bool lock; //don't focus mupen
FILE* cFile; /*combo file conains list of combos in format:
				n bytes - null terminated name string,
				4 bytes - combo length,
				length*4 bytes - combo data.*/

std::vector<COMBO> ComboList;


struct Status
{
	Status()
	{
		statusThread = NULL;
		dwThreadId = 0;
		overrideOn = false;
		overrideAllowed = false;
		relativeControlNow = false;
//		incrementingFrameNow = true;
		gettingKeys = false;
		resetXScale = false;
		resetYScale = false;
		relativeXOn = 0;
		relativeYOn = 0;
		radialAngle = -PI/2;
		skipEditX = false;
		skipEditY = false;
		xScale = 1.0f;
		yScale = 1.0f;
//		frameCounter = 0;
		buttonOverride.Value = 0;
		buttonAutofire.Value = 0;
		buttonAutofire2.Value = 0;
		buttonDisplayed.Value = 0;
		radialDistance = 0.0f;
		radialRecalc = true;
		statusDlg = NULL;
		prevHWnd = NULL;
		Extend = 1|2;
		positioned = false;
		comboTask = C_IDLE;
	}

	void StartThread(int ControllerNumber)
	{
		HANDLE prevStatusThread = statusThread;
		HWND prevStatusDlg = statusDlg;

		Control = ControllerNumber;
		dwThreadId = Control;
		dwThreadParam = MAKEWORD(Control, Extend);

		// CreateDialog() won't work, because the emulator eats our messages when it's paused
		// and can't call IsDialogMessage() because it doesn't know what our dialog is.
		// So, create a new thread and spawn a MODAL (to that thread) dialog,
		// to guarantee it always gets the messages it should.
		statusThread = CreateThread(0, 0, StatusDlgThreadProc, &dwThreadParam, 0, &dwThreadId);

		if(prevStatusDlg)
			DestroyWindow(prevStatusDlg);
		if(prevStatusThread)
			TerminateThread(prevStatusThread, 0); // last because prevStatusThread might be the currently-running thread
	}

	void StopThread()
	{
		if(statusDlg)
		{
			DestroyWindow(statusDlg);
			statusDlg = NULL;
		}
		if(statusThread)
		{
			FreeCombos(); //apparently this is a good idea
			TerminateThread(statusThread, 0);
			statusThread = NULL;
		}
	}

	void EnsureRunning()
	{
		if(!statusDlg || !statusThread || !initialized)
		{
			if(statusDlg) DestroyWindow(statusDlg), statusDlg = NULL;
			if(statusThread) TerminateThread(statusThread, 0), statusThread = NULL;
			StartThread(Control);
		}
	}

	bool HasPanel(int num) { return 0 != (Extend & (1<<(num-1))); }
	void AddPanel(int num) { Extend |= (1<<(num-1)); }
	void RemovePanel(int num) { Extend &= ~(1<<(num-1)); }

	HANDLE statusThread;
	DWORD dwThreadId, dwThreadParam;
	int overrideX, overrideY;
	bool overrideOn, overrideAllowed, relativeControlNow, gettingKeys;
//	bool incrementingFrameNow;
	DWORD relativeXOn, relativeYOn;
	float radialAngle, radialDistance, radialRecalc;
	bool dragging, draggingStick, draggingPermaStick;
	bool AngDisp;
	int dragXStart, dragYStart;
	int lastXDrag, lastYDrag;
	bool lastClick, nextClick, lastWasRight;
	bool deactivateAfterClick, skipEditX, skipEditY;
	bool positioned, initialized;
	int xPosition, yPosition;
	float xScale, yScale;
	bool resetXScale, resetYScale;
//	int frameCounter;
	static int frameCounter;
	int comboStart;
	BUTTONS buttonOverride, buttonAutofire, buttonAutofire2;
	BUTTONS buttonDisplayed;
	BUTTONS	LastControllerInput;
	HWND statusDlg;
	HWND prevHWnd;
	HWND lBox;
	int Control;
	int Extend;
	int comboTask;
	int activeCombo;

	void FreeCombos();

	void InitialiseCombos(HWND);
	void StopRecording();
	int CreateNewCombo(int);
	void StartEdit(int);
	void EndEdit(int,char*);
	void RefreshAnalogPicture ();

	void ActivateEmulatorWindow ();
	bool IsWindowFromEmulatorProcessActive ();
	static bool IsAnyStatusDialogActive ();
	LRESULT StatusDlgMethod (UINT msg, WPARAM wParam, LPARAM lParam);
	void GetKeys(BUTTONS * Keys);
	void SetKeys(BUTTONS ControllerInput);
};
int Status::frameCounter = 0;


Status status [NUMBER_OF_CONTROLS];


#define STICKPIC_SIZE (131)

int WINAPI DllMain ( HINSTANCE hInstance, DWORD fdwReason, PVOID pvReserved)
{    
	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
			g_hInstance = hInstance;
			break;

		case DLL_PROCESS_DETACH:
			FreeDirectInput();
			break;
	}

	return TRUE;
}

EXPORT void CALL CloseDLL (void)
{
	//Stop and Close Direct Input
	FreeDirectInput();
}

EXPORT void CALL ControllerCommand ( int Control, BYTE * Command)
{
}

EXPORT void CALL DllAbout ( HWND hParent )
{
	MessageBox(hParent, PLUGIN_NAME"\nFor DirectX 7 or higher\n\nBased on Def's Direct Input 0.54 by Deflection\nTAS Modifications by Nitsuja","About",MB_ICONINFORMATION | MB_OK);
}

EXPORT void CALL DllConfig ( HWND hParent )
{
	if (g_lpDI == NULL)
		InitializeAndCheckDevices(hParent);
	else
		DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_CONFIGDLG), hParent, (DLGPROC)ConfigDlgProc);

	// make config box restart the input status control if it's been closed
	if(romIsOpen)
		for(int i=0; i<NUMBER_OF_CONTROLS; i++)
			if(Controller[i].bActive)
				status[i].EnsureRunning();
			else
				status[i].StopThread();
}

EXPORT void CALL GetDllInfo ( PLUGIN_INFO * PluginInfo )
{
	PluginInfo->Version = 0x0100;
	PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
	wsprintf(PluginInfo->Name,PLUGIN_NAME);
}

EXPORT void CALL GetKeys(int Control, BUTTONS * Keys )
{
	if(Control >= 0 && Control < NUMBER_OF_CONTROLS && Controller[Control].bActive)
		status[Control].GetKeys(Keys);
	else
		Keys->Value = 0;
}

EXPORT void CALL SetKeys(int Control, BUTTONS ControllerInput)
{
	if(Control >= 0 && Control < NUMBER_OF_CONTROLS && Controller[Control].bActive)
		status[Control].SetKeys(ControllerInput);
}

LRESULT CALLBACK EditBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR sId, DWORD_PTR dwRefData)
{
	//debug messages
	//if (msg == ...)
	//{
		//char txt[100];
		//sprintf(txt, "msg: %d", msg);
		//ListBox_InsertString(GetParent(hwnd), -1, txt);
	//}

	switch (msg)
	{
	case WM_GETDLGCODE:
	{
		char txt[MAX_PATH] = "\0";
		if (wParam == VK_RETURN)
		{
			SendMessage(hwnd, WM_GETTEXT, sizeof(txt), (LPARAM)txt);
			SendMessage(GetParent(GetParent(hwnd)), EDIT_END, 0, (LPARAM)txt);
			DestroyWindow(hwnd);
		}
		else if (wParam == VK_ESCAPE)
		{
			DestroyWindow(hwnd);
		}
		break;
	}
	case WM_KILLFOCUS:
		DestroyWindow(hwnd);
		lock = false;
			break;
	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, EditBoxProc, sId);
	}
	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

//reallocate buffer to closest multiple of chunk size <- assumes it didn't overflow already
//returns pointer to new buffer (or old extended pointer)
int *ExtendBuffer(int* buf, int curSize) //size in bytes
{
	//realloc(buf, curSize + BUFFER_CHUNK); //bad because crashes on unaligned buffers, which may come from combo file
	int newsize = (div(curSize, BUFFER_CHUNK).quot+1) * BUFFER_CHUNK*4;
	return (int*)realloc(buf, newsize);
}

void Status::GetKeys(BUTTONS * Keys)
{
	if (activeCombo!=-1 && (comboTask == C_RUNNING || comboTask == C_LOOP)) //override controller data with combo
	{
		int frame = frameCounter - comboStart+1; //+1 so it's human friendly
		Keys->Value = aCombo.data[frame-1];
		SetKeys(*Keys); //show changes
		if (frame == aCombo.length) {
			if (comboTask == C_LOOP)
			{
				comboStart = frameCounter+1; //+1 because frameCounter will get incremented by the time it get's here again
			}
			else
			{
				comboTask = C_IDLE;
			}
		}
		return;
	}

	gettingKeys = true;

//	if(incrementingFrameNow)
//		frameCounter++;

	BUTTONS	ControllerInput;
	
	//Empty Keyboard Button Info 
	ControllerInput.Value = 0;
	
	if (Controller[Control].bActive == TRUE)
	{
		BYTE     buffer[256];   //Keyboard Info 
		DIJOYSTATE js;          //Joystick Info
		HRESULT  hr;
		int M1Speed = 0, M2Speed = 0;
		bool analogKey = false;

		if (Keys == NULL) { gettingKeys = false; return; }

		for(BYTE devicecount=0; devicecount<Controller[Control].NDevices; devicecount++)
		{		
			BYTE DeviceNum = (BYTE) Controller[Control].Devices[devicecount];
			if (DInputDev[DeviceNum].lpDIDevice != NULL)
			{
				LONG count;
		
				if((DInputDev[DeviceNum].DIDevInst.dwDevType & DI8DEVTYPE_KEYBOARD) == DI8DEVTYPE_KEYBOARD)
				{
					ZeroMemory( &buffer, sizeof(buffer) );	
					if FAILED(hr = DInputDev[DeviceNum].lpDIDevice->GetDeviceState(sizeof(buffer),&buffer))
					{
						hr = DInputDev[DeviceNum].lpDIDevice->Acquire();
						while( hr == DIERR_INPUTLOST ) 
							hr = DInputDev[DeviceNum].lpDIDevice->Acquire();
						gettingKeys = false;
						return;
					}
						
					for (count=0;count<NUMBER_OF_BUTTONS;count++)
					{
						if ( Controller[Control].Input[count].Device == DeviceNum )
						{
							switch ( Controller[Control].Input[count].type )
							{
								//Record Keyboard Button Info from Device State Buffer
								case INPUT_TYPE_KEY_BUT:
									if (BUTTONDOWN(buffer, Controller[Control].Input[count].vkey))
									{
										switch (count)
										{
											case 18:
												M1Speed = Controller[Control].Input[count].button; 
												break;

											case 19:
												M2Speed = Controller[Control].Input[count].button;
												break;

											case 0:
											case 1:
											case 2:
											case 3:
												analogKey = true;
												/* fall through */
											default:
												ControllerInput.Value |= Controller[Control].Input[count].button;
												break;
										}
									}
									break;

								default:
									break;
							}
						}
					}
				}

				else if((DInputDev[DeviceNum].DIDevInst.dwDevType & DI8DEVTYPE_JOYSTICK) == DI8DEVTYPE_JOYSTICK)
				{
					if FAILED(hr = DInputDev[DeviceNum].lpDIDevice->Poll())
					{
						hr = DInputDev[DeviceNum].lpDIDevice->Acquire();
						while( hr == DIERR_INPUTLOST ) 
							hr = DInputDev[DeviceNum].lpDIDevice->Acquire();
						gettingKeys = false;
						return;
					}
					if FAILED( hr = DInputDev[DeviceNum].lpDIDevice->GetDeviceState( sizeof(DIJOYSTATE), &js ) )
					{
						gettingKeys = false;
						return;
					}
				    
					for (count=0;count<NUMBER_OF_BUTTONS;count++)
					{
						if ( Controller[Control].Input[count].Device == DeviceNum )
						{
							BYTE count2;
							switch ( Controller[Control].Input[count].type ) 
							{
								//Get Joystick button Info from Device State js stucture
								case INPUT_TYPE_JOY_BUT:
									if (BUTTONDOWN(js.rgbButtons, Controller[Control].Input[count].vkey))
									{
										switch (count)
										{
											case 18:
												M1Speed = Controller[Control].Input[count].button; 
												break;

											case 19:
												M2Speed = Controller[Control].Input[count].button;
												break;

											case 0:
											case 1:
											case 2:
											case 3:
												analogKey = true;
												/* fall through */
											default:
												ControllerInput.Value |= Controller[Control].Input[count].button;
												break;
										}
									}
									break;
						
								case INPUT_TYPE_JOY_AXIS:
									switch (Controller[Control].Input[count].vkey)
									{
										case DIJOFS_YN:
											if( js.lY < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.lY, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_YP:
											if( js.lY > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.lY, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_XN:
											if( js.lX < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.lX, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_XP:
											if( js.lX > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.lX, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_ZN:
											if( js.lZ < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.lZ, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_ZP:
											if( js.lZ > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.lZ, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_RYN:
											if( js.lRy < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.lRy, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_RYP:
											if( js.lRy > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.lRy, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_RXN:
											if( js.lRx < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.lRx, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_RXP:
											if( js.lRx > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.lRx, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_RZN:
											if( js.lRz < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.lRz, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_RZP:
											if( js.lRz > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.lRz, Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_SLIDER0N:
											if( js.rglSlider[0] < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.rglSlider[0], Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_SLIDER0P:
											if( js.rglSlider[0] > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.rglSlider[0], Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_SLIDER1N:
											if( js.rglSlider[1] < (LONG) -Controller[Control].SensMin )
												GetNegAxisVal(js.rglSlider[1], Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
										case DIJOFS_SLIDER1P:
											if( js.rglSlider[1] > (LONG) Controller[Control].SensMin )
												GetPosAxisVal(js.rglSlider[1], Control, count, &ControllerInput, M1Speed, M2Speed);
											break;
									}
									break;
								
								case INPUT_TYPE_JOY_POV:
									for (count2=0;count2<NUMBER_OF_CONTROLS;count2++)
									{
										if ( (js.rgdwPOV[count2] != -1)  && (LOWORD(js.rgdwPOV[count2]) != 0xFFFF) )
										{
											switch(Controller[Control].Input[count].vkey)
											{
												case DIJOFS_POV0N:
												case DIJOFS_POV1N:
												case DIJOFS_POV2N:
												case DIJOFS_POV3N:
													if( (js.rgdwPOV[count2] >= 31500) || (js.rgdwPOV[count2] <= 4500) )
													{
														switch (count)
														{
															case 18:
																M1Speed = Controller[Control].Input[count].button; 
																break;

															case 19:
																M2Speed = Controller[Control].Input[count].button;
																break;

															case 0:
															case 1:
															case 2:
															case 3:
																analogKey = true;
																/* fall through */
															default:
																ControllerInput.Value |= Controller[Control].Input[count].button;
																break;
														}

													}
													break;
												case DIJOFS_POV0E:
												case DIJOFS_POV1E:
												case DIJOFS_POV2E:
												case DIJOFS_POV3E:
													if ( (js.rgdwPOV[count2] >= 4500) && (js.rgdwPOV[count2] <= 13500) )
													{
														switch (count2)
														{
															case 18:
																M1Speed = Controller[Control].Input[count].button; 
																break;

															case 19:
																M2Speed = Controller[Control].Input[count].button;
																break;

															case 0:
															case 1:
															case 2:
															case 3:
																analogKey = true;
																/* fall through */
															default:
																ControllerInput.Value |= Controller[Control].Input[count].button;
																break;
														}
													}
													break;
												case DIJOFS_POV0S:
												case DIJOFS_POV1S:
												case DIJOFS_POV2S:
												case DIJOFS_POV3S:
													if ( (js.rgdwPOV[count2] >= 13500) && (js.rgdwPOV[count2] <= 22500) )
													{
														switch (count2)
														{
															case 18:
																M1Speed = Controller[Control].Input[count].button; 
																break;

															case 19:
																M2Speed = Controller[Control].Input[count].button;
																break;

															case 0:
															case 1:
															case 2:
															case 3:
																analogKey = true;
																/* fall through */
															default:
																ControllerInput.Value |= Controller[Control].Input[count].button;
																break;
														}
													}
													break;
												case DIJOFS_POV0W:
												case DIJOFS_POV1W:
												case DIJOFS_POV2W:
												case DIJOFS_POV3W:
													if ( (js.rgdwPOV[count2] >= 22500) && (js.rgdwPOV[count2] <= 31500) )
													{
														switch (count2)
														{
															case 18:
																M1Speed = Controller[Control].Input[count].button; 
																break;

															case 19:
																M2Speed = Controller[Control].Input[count].button;
																break;

															case 0:
															case 1:
															case 2:
															case 3:
																analogKey = true;
																/* fall through */
															default:
																ControllerInput.Value |= Controller[Control].Input[count].button;
																break;
														}
													}
													break;
											}
										}
									}

								default:
									break;
							}
						}
					}
				}
			}
		}
		
		if(M2Speed)
		{
			if ( ControllerInput.Y_AXIS < 0 ) 
				ControllerInput.Y_AXIS = (char) -M2Speed;
			else if ( ControllerInput.Y_AXIS > 0 )
				ControllerInput.Y_AXIS = (char) M2Speed;
			
			if ( ControllerInput.X_AXIS < 0 )
				ControllerInput.X_AXIS = (char) -M2Speed;
			else if ( ControllerInput.X_AXIS > 0 )
				ControllerInput.X_AXIS = (char) M2Speed;
		}
		if(M1Speed)
		{
			if ( ControllerInput.Y_AXIS < 0 ) 
				ControllerInput.Y_AXIS = (char) -M1Speed;
			else if ( ControllerInput.Y_AXIS > 0 )
				ControllerInput.Y_AXIS = (char) M1Speed;
			
			if ( ControllerInput.X_AXIS < 0 )
				ControllerInput.X_AXIS = (char) -M1Speed;
			else if ( ControllerInput.X_AXIS > 0 )
				ControllerInput.X_AXIS = (char) M1Speed;
		}
		if(analogKey)
		{
			if(ControllerInput.X_AXIS && ControllerInput.Y_AXIS)
			{
				const static float mult = 1.0f / sqrtf(2.0f);
				float mult2;
				if(Controller[Control].SensMax > 127)
					mult2 = (float)Controller[Control].SensMax * (1.0f / 127.0f);
				else
					mult2 = 1.0f;
				if(!relativeXOn)
					ControllerInput.X_AXIS = (int)(ControllerInput.X_AXIS * mult*mult2 + (ControllerInput.X_AXIS>0 ? 0.5f : -0.5f));
				if(!relativeYOn && relativeXOn != 3)
					ControllerInput.Y_AXIS = (int)(ControllerInput.Y_AXIS * mult*mult2 + (ControllerInput.Y_AXIS>0 ? 0.5f : -0.5f));

				int newX = (int)((float)ControllerInput.X_AXIS * xScale + (ControllerInput.X_AXIS>0 ? 0.5f : -0.5f));
				int newY = (int)((float)ControllerInput.Y_AXIS * yScale + (ControllerInput.Y_AXIS>0 ? 0.5f : -0.5f));
				if(abs(newX) >= abs(newY) && (newX > 127 || newX < -128))
				{
					newY = newY * (newY>0 ? 127 : 128) / abs(newX);
					newX = (newX>0) ? 127 : -128;
				}
				else if(abs(newX) <= abs(newY) && (newY > 127 || newY < -128))
				{
					newX = newX * (newX>0 ? 127 : 128) / abs(newY);
					newY = (newY>0) ? 127 : -128;
				}
				if(!newX && ControllerInput.X_AXIS) newX = (ControllerInput.X_AXIS>0) ? 1 : -1;
				if(!newY && ControllerInput.Y_AXIS) newY = (ControllerInput.Y_AXIS>0) ? 1 : -1;
				if(!relativeXOn)
					ControllerInput.X_AXIS = newX;
				if(!relativeYOn && relativeXOn != 3)
					ControllerInput.Y_AXIS = newY;
			}
			else
			{
				if(ControllerInput.X_AXIS && !relativeXOn)
				{
					int newX = (int)((float)ControllerInput.X_AXIS * xScale + (ControllerInput.X_AXIS>0 ? 0.5f : -0.5f));
					if(!newX && ControllerInput.X_AXIS) newX = (ControllerInput.X_AXIS>0) ? 1 : -1;
					ControllerInput.X_AXIS = min(127,max(-128,newX));
				}
				if(ControllerInput.Y_AXIS && !relativeYOn && relativeXOn != 3)
				{
					int newY = (int)((float)ControllerInput.Y_AXIS * yScale + (ControllerInput.Y_AXIS>0 ? 0.5f : -0.5f));
					if(!newY && ControllerInput.Y_AXIS) newY = (ControllerInput.Y_AXIS>0) ? 1 : -1;
					ControllerInput.Y_AXIS = min(127,max(-128,newY));
				}
			}
		}
	}

	ControllerInput.Value |= (buttonOverride.Value &= ~ControllerInput.Value);
//	if((frameCounter/2)%2 == 0)
	if(frameCounter%2 == 0)
		ControllerInput.Value ^= (buttonAutofire.Value &= ~buttonOverride.Value);
	else
		ControllerInput.Value ^= (buttonAutofire2.Value &= ~buttonOverride.Value);

	bool prevOverrideAllowed = overrideAllowed;
	overrideAllowed = true;
	if (comboTask != C_PAUSE)
	{
		SetKeys(ControllerInput);
	}
	overrideAllowed = prevOverrideAllowed;

	if(overrideOn)
	{
		ControllerInput.X_AXIS = overrideX;
		ControllerInput.Y_AXIS = overrideY;
	}

	//Pass Button Info to Emulator
	Keys->Value = ControllerInput.Value;

	//copy fetched data to combo too
	if (comboTask == C_RECORD)
	{
		//extend if full and frame is not 0
		if (frameCounter-comboStart && aCombo.length%BUFFER_CHUNK==0) aCombo.data = ExtendBuffer(aCombo.data, aCombo.length*4);
		aCombo.data[frameCounter - comboStart] = Keys->Value;
		aCombo.length++;
	}

	gettingKeys = false;
}

void Status::SetKeys(BUTTONS ControllerInput)
{
	bool changed = false;
	if(statusDlg)
	{
		if(buttonDisplayed.Value != ControllerInput.Value)
		{
			if(HasPanel(2))
			{
#define UPDATECHECK(idc,field) {if(buttonDisplayed.field != ControllerInput.field) CheckDlgButton(statusDlg, idc, ControllerInput.field);}
				UPDATECHECK(IDC_CHECK_A, A_BUTTON);
				UPDATECHECK(IDC_CHECK_B, B_BUTTON);
				UPDATECHECK(IDC_CHECK_START, START_BUTTON);
				UPDATECHECK(IDC_CHECK_L, L_TRIG);
				UPDATECHECK(IDC_CHECK_R, R_TRIG);
				UPDATECHECK(IDC_CHECK_Z, Z_TRIG);
				UPDATECHECK(IDC_CHECK_CUP, U_CBUTTON);
				UPDATECHECK(IDC_CHECK_CLEFT, L_CBUTTON);
				UPDATECHECK(IDC_CHECK_CRIGHT, R_CBUTTON);
				UPDATECHECK(IDC_CHECK_CDOWN, D_CBUTTON);
				UPDATECHECK(IDC_CHECK_DUP, U_DPAD);
				UPDATECHECK(IDC_CHECK_DLEFT, L_DPAD);
				UPDATECHECK(IDC_CHECK_DRIGHT, R_DPAD);
				UPDATECHECK(IDC_CHECK_DDOWN, D_DPAD);
#undef UPDATECHECK
			}
			buttonDisplayed.Value = ControllerInput.Value;
		}

		if(relativeXOn == 3 && radialRecalc)
		{
			radialDistance = sqrtf((float)((overrideX*overrideX)/(xScale*xScale) + (overrideY*overrideY)/(yScale*yScale)));
			if(radialDistance != 0)
				radialAngle = atan2f((float)-(overrideY/yScale), (float)(overrideX/xScale));
			radialRecalc = false;
		}

		// relative change amount
		int addX = (int)((ControllerInput.X_AXIS * xScale) / 12.0f);
		int addY = (int)((ControllerInput.Y_AXIS * yScale) / 12.0f);

		// calculate fractional (over time) change
		if(relativeControlNow && relativeXOn && ControllerInput.X_AXIS)
		{
			static float incrX = 0.0f;
			incrX += ((ControllerInput.X_AXIS * xScale) / 12.0f) - addX;
			if(incrX > 1.0f)
				addX++, incrX -= 1.0f;
			else if(incrX < -1.0f)
				addX--, incrX += 1.0f;
		}
		if(relativeControlNow && (relativeYOn || relativeXOn==3) && ControllerInput.Y_AXIS)
		{
			static float incrY = 0.0f;
			incrY += ((ControllerInput.Y_AXIS * yScale) / 12.0f) - addY;
			if(incrY > 1.0f)
				addY++, incrY -= 1.0f;
			else if(incrY < -1.0f)
				addY--, incrY += 1.0f;
		}

		if(relativeXOn && overrideAllowed)
		{
			if(relativeControlNow && ControllerInput.X_AXIS) // increment x position by amount relative to attempted new x position
			{
				if(relativeXOn != 3)
				{
					int nextVal = overrideX + addX;
					if(nextVal <= 127 && nextVal >= -128)
					{
						if(!overrideX || (relativeXOn==1 ? (((overrideX<0) == (ControllerInput.X_AXIS<0)) || !ControllerInput.X_AXIS) : ((overrideX<0) != (nextVal>0))))
							ControllerInput.X_AXIS = nextVal;
						else
							ControllerInput.X_AXIS = 0; // lock to 0 once on crossing +/- boundary, or "semi-relative" jump to 0 on direction reversal
					}
					else
					{
						ControllerInput.X_AXIS = nextVal>0 ? 127 : -128;
						if(abs(overrideY + addY) < 129)
							overrideY = overrideY * 127 / abs(nextVal);
					}
				}
				else // radial mode, angle change
				{
					radialAngle += 4*PI; // keeping it positive
					float oldAngle = fmodf(radialAngle, 2*PI);
					radialAngle = fmodf(radialAngle + (ControllerInput.X_AXIS) / (250.0f + 500.0f/(sqrtf(xScale*yScale)+0.001f)), 2*PI);

					// snap where crossing 45 degree boundaries
					if(radialAngle-oldAngle > PI) radialAngle -= 2*PI;
					else if(oldAngle-radialAngle > PI) oldAngle -= 2*PI;
					float oldDeg = oldAngle*(180.0f/PI);
					float newDeg = radialAngle*(180.0f/PI);
					for(float ang = 0 ; ang < 360 ; ang += 45)
						if(fabsf(oldDeg - ang) > (0.0001f) && (oldDeg < ang) != (newDeg < ang))
						{
							radialAngle = ang*(PI/180.0f);
							break;
						}

					float xAng = xScale * radialDistance * cosf(radialAngle);
					float yAng = yScale * radialDistance * sinf(radialAngle);
					int newX =  (int)(xAng + (xAng>0 ? 0.5f : -0.5f));
					int newY = -(int)(yAng + (yAng>0 ? 0.5f : -0.5f));
					if(newX > 127) newX = 127;
					if(newX < -128) newX = -128;
					if(newY > 127) newY = 127;
					if(newY < -128) newY = -128;
					ControllerInput.X_AXIS = newX;
					overrideY              = newY;
				}
			}
			else
				ControllerInput.X_AXIS = overrideX;
		}
		if((relativeYOn || (relativeXOn == 3)) && overrideAllowed)
		{
			if(relativeControlNow && (ControllerInput.Y_AXIS || (!relativeYOn && ControllerInput.Y_AXIS != LastControllerInput.Y_AXIS))) // increment y position by amount relative to attempted new y position
			{
				if(relativeXOn != 3) // not a typo (relativeXOn holds radial mode setting)
				{
					int nextVal = overrideY + addY;
					if(nextVal <= 127 && nextVal >= -128)
					{
						if(!overrideY || (relativeYOn==1 ? (((overrideY<0) == (ControllerInput.Y_AXIS<0)) || !ControllerInput.Y_AXIS) : ((overrideY<0) != (nextVal>0))))
							ControllerInput.Y_AXIS = nextVal;
						else
							ControllerInput.Y_AXIS = 0; // lock to 0 once on crossing +/- boundary, or "semi-relative" jump to 0 on direction reversal
					}
					else
					{
						if(abs(overrideX + addX) < 129)
							ControllerInput.X_AXIS = ControllerInput.X_AXIS * 127 / abs(nextVal);
						ControllerInput.Y_AXIS = nextVal>0 ? 127 : -128;
					}
				}
				else // radial mode, length change
				{
					if(!relativeYOn) // relative rotation + instant distance
					{
						if(ControllerInput.Y_AXIS < 0)
							radialDistance = 0;
						else if(!Controller[Control].SensMin || abs(ControllerInput.Y_AXIS) >= Controller[Control].SensMin)
						{
							radialDistance = (float)ControllerInput.Y_AXIS;
							if(radialDistance == 127) radialDistance = 128;
						}
						else
							radialDistance = (float)Controller[Control].SensMin;
						LastControllerInput.Y_AXIS = ControllerInput.Y_AXIS;
					}
					else if(relativeYOn == 1 && radialDistance && (radialDistance>0) != (ControllerInput.Y_AXIS>0))
						radialDistance = 0; // "semi-relative" distance, jump to 0 on direction reversal
					else
						radialDistance += addY;
					const static float maxDist = sqrtf(128*128+128*128);
					if(radialDistance > maxDist) radialDistance = maxDist;
					float xAng = xScale * radialDistance * cosf(radialAngle);
					float yAng = yScale * radialDistance * sinf(radialAngle);
					int newX =  (int)(xAng + (xAng>0 ? 0.5f : -0.5f));
					int newY = -(int)(yAng + (yAng>0 ? 0.5f : -0.5f));
					if(newX > 127) newX = 127;
					if(newX < -128) newX = -128;
					if(newY > 127) newY = 127;
					if(newY < -128) newY = -128;
					ControllerInput.X_AXIS = newX;
					ControllerInput.Y_AXIS = newY;
				}
			}
			else
				ControllerInput.Y_AXIS = overrideY;
		}
		if(LastControllerInput.X_AXIS != ControllerInput.X_AXIS || (AngDisp && LastControllerInput.Y_AXIS != ControllerInput.Y_AXIS))
		{
			if(HasPanel(1))
			{
				char str [256], str2 [256];
				GetDlgItemText(statusDlg, IDC_EDITX, str2, 256);
				if(!AngDisp)
					sprintf(str, "%d", ControllerInput.X_AXIS);
				else
				{
					float radialAngle = 4*PI + atan2f(((float)-ControllerInput.Y_AXIS)/yScale, ((float)ControllerInput.X_AXIS)/xScale);
					float angle2 = fmodf(90.0f + radialAngle*(180.0f/PI), 360.0f);
					sprintf(str, "%d", (int)(angle2 + (angle2>0 ? 0.5f : -0.5f)));
					skipEditX = true;
					overrideX = (int)ControllerInput.X_AXIS;
					RefreshAnalogPicture();
				}
				if(strcmp(str,str2))
					SetDlgItemText(statusDlg, IDC_EDITX, str);
			}
			changed = true;
		}
		if(LastControllerInput.Y_AXIS != ControllerInput.Y_AXIS || (AngDisp && LastControllerInput.X_AXIS != ControllerInput.X_AXIS))
		{
			if(HasPanel(1))
			{
				char str [256], str2 [256];
				GetDlgItemText(statusDlg, IDC_EDITY, str2, 256);
				if(!AngDisp)
					sprintf(str, "%d", -ControllerInput.Y_AXIS);
				else
				{
					float radialDistance = sqrtf(((float)(ControllerInput.X_AXIS*ControllerInput.X_AXIS)/(xScale*xScale) + (float)(ControllerInput.Y_AXIS*ControllerInput.Y_AXIS)/(yScale*yScale)));
					sprintf(str, "%d", (int)(0.5f + radialDistance));
					skipEditY = true;
					overrideY = (int)ControllerInput.Y_AXIS;
					RefreshAnalogPicture();
				}
				if(strcmp(str,str2))
					SetDlgItemText(statusDlg, IDC_EDITY, str);
			}
			changed = true;
		}
	}
	if(relativeYOn || relativeXOn != 3)
		LastControllerInput = ControllerInput;

	if(changed)
	{
		overrideX = ControllerInput.X_AXIS;
		overrideY = ControllerInput.Y_AXIS;
		RefreshAnalogPicture();
	}
}


void WINAPI GetNegAxisVal(LONG AxisValue, int Control, LONG count, BUTTONS *ControllerInput, int &M1Speed, int &M2Speed)
{
	switch(count)
	{
		case 0:
			if ( AxisValue < (LONG) -Controller[Control].SensMax)
				ControllerInput->Y_AXIS = min(127,Controller[Control].SensMax);
			else
				ControllerInput->Y_AXIS = -AxisValue; 
			break;
		case 1:
			if ( AxisValue < (LONG) -Controller[Control].SensMax)
				ControllerInput->Y_AXIS = -min(128,Controller[Control].SensMax);
			else
				ControllerInput->Y_AXIS = AxisValue;
			break;
		case 2:
			if ( AxisValue < (LONG) -Controller[Control].SensMax)
				ControllerInput->X_AXIS = -min(128,Controller[Control].SensMax);
			else
				ControllerInput->X_AXIS = AxisValue; 
			break;
		case 3:
			if ( AxisValue < (LONG) -Controller[Control].SensMax)
				ControllerInput->X_AXIS = min(127,Controller[Control].SensMax);
			else
				ControllerInput->X_AXIS = -AxisValue; 
			break;
		
		case 18:
			M1Speed = Controller[Control].Input[count].button; 
			break;
		case 19:
			M2Speed = Controller[Control].Input[count].button;
			break;

		default: 
			ControllerInput->Value |= Controller[Control].Input[count].button; 
			break;
	}
}

void WINAPI GetPosAxisVal(LONG AxisValue, int Control, LONG count, BUTTONS *ControllerInput, int &M1Speed, int &M2Speed) {
	switch(count)
	{
		case 0:
			if ( AxisValue > (LONG) Controller[Control].SensMax)
				ControllerInput->Y_AXIS = min(127,Controller[Control].SensMax);
			else
				ControllerInput->Y_AXIS = AxisValue;
			break;
		case 1:
			if ( AxisValue > (LONG) Controller[Control].SensMax)
				ControllerInput->Y_AXIS = -min(128,Controller[Control].SensMax);
			else
				ControllerInput->Y_AXIS = -AxisValue; 
			break;
		case 2:
			if ( AxisValue > (LONG) Controller[Control].SensMax)
				ControllerInput->X_AXIS = -min(128,Controller[Control].SensMax);
			else
				ControllerInput->X_AXIS = -AxisValue; 
			break;
		case 3:
			if ( AxisValue > (LONG) Controller[Control].SensMax)
				ControllerInput->X_AXIS = min(127,Controller[Control].SensMax);
			else
				ControllerInput->X_AXIS = AxisValue; 
			break;
		
		case 18:
			M1Speed = Controller[Control].Input[count].button; 
			break;
		case 19:
			M2Speed = Controller[Control].Input[count].button;
			break;

		default: 
			ControllerInput->Value |= Controller[Control].Input[count].button; 
			break;
	}
}


EXPORT void CALL InitiateControllers (HWND hMainWindow, CONTROL Controls[4]) {
	HKEY hKey;
	DWORD dwSize, dwType;

	for (BYTE i=0; i<NUMBER_OF_CONTROLS; i++) {
		ControlDef[i] = &Controls[i];
		ControlDef[i]->Present = FALSE;
		ControlDef[i]->RawData = FALSE;
		ControlDef[i]->Plugin  = PLUGIN_NONE;

		Controller[i].NDevices = 0;
		Controller[i].bActive = i==0 ? TRUE : FALSE;
		Controller[i].SensMax = 128;
		Controller[i].SensMin = 32;
		Controller[i].Input[18].button = 42;
		Controller[i].Input[19].button = 20;
		wsprintf(Controller[i].szName,"Controller %d",i+1);		
	}

	if (g_lpDI == NULL) {
		InitializeAndCheckDevices(hMainWindow);
	}
	
	dwType = REG_BINARY;
	dwSize = sizeof(DEFCONTROLLER);

	if ( RegCreateKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, 0) != ERROR_SUCCESS ) {
		MessageBox(NULL, "Could not create Registry Key", "Error", MB_ICONERROR | MB_OK);
	}
	else {
		for (BYTE NController = 0; NController<NUMBER_OF_CONTROLS; NController++) {
			if ( RegQueryValueEx(hKey, Controller[NController].szName, 0, &dwType, (LPBYTE)&Controller[NController], &dwSize) == ERROR_SUCCESS ) {
				if ( Controller[NController].bActive )
					ControlDef[NController]->Present = TRUE;
				else
					ControlDef[NController]->Present = FALSE;
				
				if ( Controller[NController].bMemPak )
					ControlDef[NController]->Plugin = PLUGIN_MEMPAK;
				else
					ControlDef[NController]->Plugin = PLUGIN_NONE;

				if ( dwSize != sizeof(DEFCONTROLLER) ) {
					dwType = REG_BINARY;
					dwSize = sizeof(DEFCONTROLLER);
					ZeroMemory( &Controller[NController], sizeof(DEFCONTROLLER) );
					
					Controller[NController].NDevices = 0;
					Controller[NController].bActive = NController==0 ? TRUE : FALSE;
					ControlDef[NController]->Present = FALSE;
					ControlDef[NController]->Plugin  = PLUGIN_NONE;
					Controller[NController].SensMax = 128;
					Controller[NController].SensMin = 32;
					Controller[NController].Input[18].button = 42;
					Controller[NController].Input[19].button = 20;
					wsprintf(Controller[NController].szName,"Controller %d",NController+1);

					RegDeleteValue(hKey, Controller[NController].szName);
					RegSetValueEx(hKey, Controller[NController].szName, 0, dwType, (LPBYTE)&Controller[NController], dwSize);
				}
			}
			else {
				dwType = REG_BINARY;
				dwSize = sizeof(DEFCONTROLLER);
				RegDeleteValue(hKey, Controller[NController].szName);
				RegSetValueEx(hKey, Controller[NController].szName, 0, dwType, (LPBYTE)&Controller[NController], dwSize);
			}
		}		
	}
	RegCloseKey(hKey);
}

void WINAPI InitializeAndCheckDevices(HWND hMainWindow) {
	HKEY hKey;
	BYTE i;
	DWORD dwSize, dwType;
	
	//Initialize Direct Input function
	if( FAILED(InitDirectInput(hMainWindow)) ) {
		MessageBox(NULL,"DirectInput Initialization Failed!","Error",MB_ICONERROR | MB_OK);
		FreeDirectInput();
	}
	else {
		dwType = REG_BINARY;
		dwSize = sizeof(Guids);
		//Check Guids for Device Changes
		RegCreateKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, 0);
		if( RegQueryValueEx(hKey, TEXT("Guids"), 0, &dwType, (LPBYTE)Guids, &dwSize) != ERROR_SUCCESS ) {
			for(i=0; i<MAX_DEVICES; i++) {
				if ( DInputDev[i].lpDIDevice == NULL )
					ZeroMemory(&Guids[i], sizeof(GUID));
				else	
					memcpy(&Guids[i], &DInputDev[i].DIDevInst.guidInstance, sizeof(GUID));
			}
			dwType = REG_BINARY;
			RegSetValueEx(hKey, TEXT("Guids"), 0, dwType, (LPBYTE)Guids, dwSize);
		}
		else {
			if( CheckForDeviceChange(hKey) ) {
				for(i=0; i<MAX_DEVICES; i++) {
					if ( DInputDev[i].lpDIDevice == NULL )
						ZeroMemory(&Guids[i], sizeof(GUID));
					else	
						memcpy(&Guids[i], &DInputDev[i].DIDevInst.guidInstance, sizeof(GUID));
				}
				dwType = REG_BINARY;
				RegSetValueEx(hKey, TEXT("Guids"), 0, dwType, (LPBYTE)Guids, dwSize);
			}
		}
		RegCloseKey(hKey);
	}
}

BOOL WINAPI CheckForDeviceChange(HKEY hKey) {
	BOOL DeviceChanged;
	DWORD dwSize, dwType;
	
	dwType = REG_BINARY;
	dwSize = sizeof(DEFCONTROLLER);

	DeviceChanged = FALSE;
	
	for(BYTE DeviceNumCheck=0; DeviceNumCheck<MAX_DEVICES; DeviceNumCheck++) {
		if( memcmp(&Guids[DeviceNumCheck], &DInputDev[DeviceNumCheck].DIDevInst.guidInstance, sizeof(GUID)) != 0) {
			DeviceChanged = TRUE;			
			for( BYTE NController=0; NController<NUMBER_OF_CONTROLS; NController++) {
				RegQueryValueEx(hKey, Controller[NController].szName, 0, &dwType, (LPBYTE)&Controller[NController], &dwSize);
				for( BYTE DeviceNum=0; DeviceNum<Controller[NController].NDevices; DeviceNum++) {
					if( Controller[NController].Devices[DeviceNum] == DeviceNumCheck) {
						Controller[NController].NDevices = 0; 
						Controller[NController].bActive = FALSE;
						RegSetValueEx(hKey, Controller[NController].szName, 0, dwType, (LPBYTE)&Controller[NController], dwSize);
					}
				}
			}
		}
	}

	return DeviceChanged;
}

EXPORT void CALL ReadController ( int Control, BYTE * Command )
{
	// XXX: Increment frame counter here because the plugin specification provides no means of finding out when a frame goes by.
	//      Mupen64 calls ReadController(-1) every input frame, but other emulators might not do that.
	//      (The frame counter is used only for autofire and combo progression.)
	if(Control == -1)
		Status::frameCounter++;
//		for(Control = 0; Control < NUMBER_OF_CONTROLS; Control++)
//			if(Controller[Control].bActive)
//				status[Control].frameCounter++;
}

EXPORT void CALL RomClosed (void) {
	romIsOpen = false;
	for(int i=0; i<NUMBER_OF_CONTROLS; i++)
		status[i].StopThread();
}


EXPORT void CALL RomOpen (void) {
	RomClosed();
	romIsOpen = true;

	HKEY hKey;
	DWORD dwSize, dwType, dwDWSize, dwDWType;

	dwType = REG_BINARY;
	dwSize = sizeof(DEFCONTROLLER);
	dwDWType = REG_DWORD;
	dwDWSize = sizeof(DWORD);
	
	if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS ) {
		for ( BYTE Control = 0; Control<NUMBER_OF_CONTROLS; Control++ ) {
			RegQueryValueEx(hKey, Controller[Control].szName, 0, &dwType, (LPBYTE)&Controller[Control], &dwSize);
			if ( Controller[Control].bActive )
				ControlDef[Control]->Present = TRUE;
			else
				ControlDef[Control]->Present = FALSE;

			char str [256];
			sprintf(str, "Controller%dRelativeX", Control);
			RegQueryValueEx(hKey, str, 0, &dwDWType, (LPBYTE)&status[Control].relativeXOn, &dwDWSize);
			sprintf(str, "Controller%dRelativeY", Control);
			RegQueryValueEx(hKey, str, 0, &dwDWType, (LPBYTE)&status[Control].relativeYOn, &dwDWSize);
			sprintf(str, "Controller%dDlgExtend", Control);
			RegQueryValueEx(hKey, str, 0, &dwDWType, (LPBYTE)&status[Control].Extend, &dwDWSize);
			sprintf(str, "Controller%dAngDisp", Control);
			RegQueryValueEx(hKey, str, 0, &dwDWType, (LPBYTE)&status[Control].AngDisp, &dwDWSize);
		}
	}
	RegCloseKey(hKey);

	for(int i=0; i<NUMBER_OF_CONTROLS; i++)
		if(Controller[i].bActive)
			status[i].StartThread(i);
		else
			status[i].Control = i;
}

EXPORT void CALL WM_KeyDown( WPARAM wParam, LPARAM lParam )
{
}

EXPORT void CALL WM_KeyUp( WPARAM wParam, LPARAM lParam )
{
}

//stop recording combo, uhh I think I dont need to do anything lol
void Status::StopRecording() {
	return;
}

void Status::FreeCombos()
{
	if (ComboList.size()) //reset
	{
		for (COMBO c : ComboList)
		{
			c.ClearData();
			//delete c; //no you can't
		}
		ComboList.clear();
	}
}

//Creates new combo, if id!=1 duplicates existing (todo)
int Status::CreateNewCombo(int id) {
	COMBO Combo;
	Combo.length = 0;
	Combo.data = (int*)malloc(BUFFER_CHUNK * 4); //must be aligned
	char name[] = "New Combo"; //must be later renamed lol
	ComboList.push_back(Combo);
	return ListBox_InsertString(lBox,-1, name);
}

//shows edit box
void Status::StartEdit(int id) {
	RECT edit;//, box;
	char txt[MAX_PATH];
	ListBox_GetItemRect(lBox, id, &edit);
	//If editBox isn't child of listbox (which would solve padding issues, but doesn't quite work)
	//GetWindowRect(lBox, &box);
	//POINT pt = { box.left, box.top };
	//ScreenToClient(statusDlg, &pt);
	//edit.top += pt.y;
	//edit.left += pt.x;
	//edit.bottom += pt.y;
	//edit.right += pt.x;
	lock = true;
	HWND editBox = CreateWindowEx(WS_EX_CLIENTEDGE,"EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP, edit.left, edit.top, edit.right - edit.left, edit.bottom - edit.top+4, lBox, 0, g_hInstance, 0);
	ListBox_SetCurSel(lBox, -1); //just to be safe, it sometimes can be seen in background

										 //FW_BOLD
	//HFONT font = CreateFont(12, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, "MS Shell Dlg 2");
	SendMessage(editBox,WM_SETFONT,(WPARAM)SendMessage(lBox, WM_GETFONT, 0, 0),0);
	SetWindowSubclass(editBox,EditBoxProc,0,0); //to add more functionality
	ListBox_GetText(lBox, activeCombo, txt);
	SendMessage(editBox, WM_SETTEXT, 0, (LPARAM)txt);
	PostMessage(statusDlg, WM_NEXTDLGCTL, (WPARAM)editBox, TRUE);
}

void Status::EndEdit(int id,char* name) {
	if (name != NULL)
	{
		ListBox_DeleteString(lBox, id);
		if (name[0] == NULL)
		{
			
			ComboList.erase(ComboList.begin()+id);
		}
		else
		{
			ListBox_InsertString(lBox,id, name);
		}
	}
}

//load combos to listBox
void Status::InitialiseCombos(HWND lBox) {
	char c; //used to read name of combo
	char name[MAX_PATH] = "Empty"; //cap of 260 characters!
	cFile = fopen("combos.txt", "a+"); //in .exe location
	c = fgetc(cFile);
	if (c==-1) { //file empty
		ListBox_AddString(lBox, name);
	}
	else {
		while (c != -1) { //for each combo
			COMBO combo;
			int i = 0;
			while (c > 0) {  //read name until \0
				name[i] = c;
				c = fgetc(cFile);
				i++;
			}
			name[i++] = '\0';
			//todo: use CreateNewCombo() here?
			ListBox_AddString(lBox, name);
			fread(&combo.length, 4, 1, cFile); //reads 4 bytes - length
			combo.data = (int*) malloc(combo.length*4);
			fread(combo.data, 4, combo.length, cFile);
			ComboList.push_back(combo); //this calls destructor
			c = fgetc(cFile);
		}
	}
	fclose(cFile);
}

void Status::RefreshAnalogPicture ()
{
	if(HasPanel(1))
	{
		RECT rect, rect2;
		GetWindowRect(GetDlgItem(statusDlg,IDC_STICKPIC), &rect);
		GetWindowRect(statusDlg, &rect2);
		rect.left -= rect2.left+3; rect.right  -= rect2.left+3;
		rect.top  -= rect2.top+3;  rect.bottom -= rect2.top+3;
		InvalidateRect(statusDlg, &rect, TRUE);
		UpdateWindow(statusDlg);
	}
}


DWORD WINAPI StatusDlgThreadProc (LPVOID lpParameter)
{
	int Control = LOBYTE(*(int*)lpParameter);
	int DialogID;
	switch(HIBYTE(*(int*)lpParameter)) // Extend
	{
		default:
		case 1:     DialogID = IDD_STATUS_1; break;
		case 1|2:   DialogID = IDD_STATUS_12; break;
		case   2:   DialogID = IDD_STATUS_2; break;
		case 1|2|4: DialogID = IDD_STATUS_123; break;
		case 1 | 4: DialogID = IDD_STATUS_13; break;
		case   2|4: DialogID = IDD_STATUS_23; break;
		case     4: DialogID = IDD_STATUS_3; break;
	}
	switch(Control)
	{
		case 0: DialogBox(g_hInstance, MAKEINTRESOURCE(DialogID), NULL, (DLGPROC)StatusDlgProc0); break;
		case 1: DialogBox(g_hInstance, MAKEINTRESOURCE(DialogID), NULL, (DLGPROC)StatusDlgProc1); break;
		case 2: DialogBox(g_hInstance, MAKEINTRESOURCE(DialogID), NULL, (DLGPROC)StatusDlgProc2); break;
		case 3: DialogBox(g_hInstance, MAKEINTRESOURCE(DialogID), NULL, (DLGPROC)StatusDlgProc3); break;
		default: break;
	}
//	DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_STATUS), NULL, (DLGPROC)StatusDlgProc);
//	statusThread = NULL;
	return 0;
}

static bool IsMouseOverControl (HWND hDlg, int dialogItemID)
{
	POINT pt;
    RECT rect;
    
    GetCursorPos(&pt);
    GetWindowRect(GetDlgItem(hDlg, dialogItemID), &rect);
    
    return (pt.x <= rect.right && pt.x >= rect.left && pt.y <= rect.bottom && pt.y >= rect.top);
}

void Status::ActivateEmulatorWindow ()
{
	if (lock) return;
	SetFocus(NULL);

	SetActiveWindow(NULL); // activates whatever the previous window was

	HWND hWnd = GetForegroundWindow();
	for(int i=0; i<NUMBER_OF_CONTROLS; i++)
		if(hWnd == status[i].statusDlg)
			hWnd = NULL;
	if(!hWnd)
	{
		if(prevHWnd)
			SetForegroundWindow(prevHWnd); // if that failed, try the last other window from the same process
		return;
	}

	// try to find out which window belongs to the emulator (the input plugin specifications provide no means of getting this)
	DWORD processID1, processID2;
	GetWindowThreadProcessId(statusDlg, &processID1);
	GetWindowThreadProcessId(hWnd, &processID2);
	if(processID1 == processID2)
		prevHWnd = hWnd; // if it's from the same process, remember it (even if we already did this, in case the emulator has multiple windows)
	else if(prevHWnd)
		SetForegroundWindow(prevHWnd); // if it's not, the last other window from the same process would be a better match
}

bool Status::IsWindowFromEmulatorProcessActive ()
{
	HWND hWnd = GetForegroundWindow();
	for(int i=0; i<NUMBER_OF_CONTROLS; i++)
		if(hWnd == status[i].statusDlg)
			hWnd = NULL;
	if(!hWnd) return true;
	DWORD processID1, processID2;
	GetWindowThreadProcessId(statusDlg, &processID1);
	GetWindowThreadProcessId(hWnd, &processID2);
	return (processID1 == processID2);
}

bool Status::IsAnyStatusDialogActive ()
{
	HWND hWnd = GetForegroundWindow();
	if(!hWnd) return false;
	for(int i=0; i<NUMBER_OF_CONTROLS; i++)
		if(hWnd == status[i].statusDlg)
			return true;
	return false;
}

//LRESULT CALLBACK StatusDlgProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
//{
//	// awkward way of getting our parameter, as this is a callback function...
//	int i;
//	if(msg == WM_INITDIALOG)
//	{
//		i = lParam;
//		status[i].statusDlg = hDlg;
//	}
//	else
//	{
//		for(i=0; i<NUMBER_OF_CONTROLS; i++)
//			if(hDlg == status[i].statusDlg)
//				break;
//		if(i == NUMBER_OF_CONTROLS)
//		    return DefWindowProc (hDlg, msg, wParam, lParam);
//	}
//
//	return status[i].StatusDlgMethod(msg, wParam, lParam);
//}

#define MAKE_DLG_PROC(i) \
LRESULT CALLBACK StatusDlgProc##i (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) \
{ \
	status[i].statusDlg = hDlg; \
	return status[i].StatusDlgMethod(msg, wParam, lParam); \
}
MAKE_DLG_PROC(0)
MAKE_DLG_PROC(1)
MAKE_DLG_PROC(2)
MAKE_DLG_PROC(3)


LRESULT Status::StatusDlgMethod (UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(initialized || msg == WM_INITDIALOG /*|| msg == WM_DESTROY || msg == WM_NCDESTROY*/)
	switch (msg)
    {
		case WM_ERASEBKGND:
			if (erase)
			{
				erase = false;
				break;
			}
			return 1;

        case WM_INITDIALOG:
		{
			// reset some dialog state
			erase = true;
			dragging = false;
			lastXDrag = 0;
			lastYDrag = 0;
			dragging = false, draggingStick = false, draggingPermaStick = false;
			dragXStart = 0, dragYStart = 0;
			lastXDrag = 0, lastYDrag = 0;
			xScale = 1.0f;
			yScale = 1.0f;
			nextClick = false;
			deactivateAfterClick = false;
			lastClick = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000));
			lastWasRight = 0!=(GetAsyncKeyState(VK_RBUTTON) & 0x8000);
			int initialStickX = overrideOn ? overrideX : LastControllerInput.X_AXIS;
			int initialStickY = overrideOn ? overrideY : LastControllerInput.Y_AXIS;

			// set title
			char str [256];
			if(HasPanel(1))
			{
				sprintf(str, "Analog Stick - Controller %d", Control+1);
				SetWindowText(GetDlgItem(statusDlg,IDC_ANALOGSTICKLABEL), str);
			}
			else if(HasPanel(2))
			{
				sprintf(str, "Buttons - Controller %d", Control+1);
				SetWindowText(GetDlgItem(statusDlg,IDC_BUTTONSLABEL), str);
			}
			else if(HasPanel(3))
			{
				sprintf(str, "Combos - NOT YET IMPLEMENTED - Controller %d", Control+1); // XXX
				SetWindowText(GetDlgItem(statusDlg,IDC_COMBOLABEL), str);
			}
			if(AngDisp)
			{
				CheckDlgButton(statusDlg, IDC_CHECK_ANGDISP, TRUE);
				SetDlgItemText(statusDlg, IDC_STATICX, "a");
				SetDlgItemText(statusDlg, IDC_STATICY, "D");
			}
			else
			{
				CheckDlgButton(statusDlg, IDC_CHECK_ANGDISP, FALSE);
				SetDlgItemText(statusDlg, IDC_STATICX, "X");
				SetDlgItemText(statusDlg, IDC_STATICY, "Y");
			}

			// calculate rects
			RECT dlgRect, picRect;
			GetWindowRect(statusDlg, &dlgRect);
			GetWindowRect(GetDlgItem(statusDlg,IDC_STICKPIC), &picRect);
			picRect.left -= dlgRect.left; picRect.right -= dlgRect.left;
			picRect.top -= dlgRect.top; picRect.bottom -= dlgRect.top;

			// move window into position
			if(!positioned)
			{
				xPosition = Control * (dlgRect.right-dlgRect.left);
				yPosition = 0;
				positioned = true;
			}
			SetWindowPos(statusDlg,HWND_NOTOPMOST, xPosition, yPosition, 0,0, SWP_NOSIZE|SWP_SHOWWINDOW);

			// reposition stick picture
			MoveWindow(GetDlgItem(statusDlg,IDC_STICKPIC), picRect.left, picRect.top, STICKPIC_SIZE, STICKPIC_SIZE, FALSE);

			// set ranges
			if(!AngDisp)
			{
				SendDlgItemMessage(statusDlg, IDC_SPINX, UDM_SETRANGE, 0, (LPARAM)MAKELONG(-128,127));
				SendDlgItemMessage(statusDlg, IDC_SPINY, UDM_SETRANGE, 0, (LPARAM)MAKELONG(-128,127));
			}
			else
			{
				SendDlgItemMessage(statusDlg, IDC_SPINX, UDM_SETRANGE, 0, (LPARAM)MAKELONG(720,-720));
				SendDlgItemMessage(statusDlg, IDC_SPINY, UDM_SETRANGE, 0, (LPARAM)MAKELONG(180,-180));
			}
			SendDlgItemMessage(statusDlg, IDC_SLIDERX, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(10,2010));
			SendDlgItemMessage(statusDlg, IDC_SLIDERX, TBM_SETPOS, TRUE, (LPARAM)(LONG)(xScale*1000));
			SendDlgItemMessage(statusDlg, IDC_SLIDERY, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(10,2010));
			SendDlgItemMessage(statusDlg, IDC_SLIDERY, TBM_SETPOS, TRUE, (LPARAM)(LONG)(yScale*1000));

			// set checkbox initial states
			if(HasPanel(1))
			{
				CheckDlgButton(statusDlg, IDC_XABS, relativeXOn==0 ? TRUE : FALSE);
				CheckDlgButton(statusDlg, IDC_YABS, relativeYOn==0 ? TRUE : FALSE);
				CheckDlgButton(statusDlg, IDC_XSEM, relativeXOn==1 ? TRUE : FALSE);
				CheckDlgButton(statusDlg, IDC_YSEM, relativeYOn==1 ? TRUE : FALSE);
				CheckDlgButton(statusDlg, IDC_XREL, relativeXOn==2 ? TRUE : FALSE);
				CheckDlgButton(statusDlg, IDC_YREL, relativeYOn==2 ? TRUE : FALSE);
				CheckDlgButton(statusDlg, IDC_XRAD, relativeXOn==3 ? TRUE : FALSE);
			}
			if(HasPanel(2))
			{
				CheckDlgButton(statusDlg, IDC_CHECK_A, buttonDisplayed.A_BUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_B, buttonDisplayed.B_BUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_START, buttonDisplayed.START_BUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_L, buttonDisplayed.L_TRIG);
				CheckDlgButton(statusDlg, IDC_CHECK_R, buttonDisplayed.R_TRIG);
				CheckDlgButton(statusDlg, IDC_CHECK_Z, buttonDisplayed.Z_TRIG);
				CheckDlgButton(statusDlg, IDC_CHECK_CUP, buttonDisplayed.U_CBUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_CLEFT, buttonDisplayed.L_CBUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_CRIGHT, buttonDisplayed.R_CBUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_CDOWN, buttonDisplayed.D_CBUTTON);
				CheckDlgButton(statusDlg, IDC_CHECK_DUP, buttonDisplayed.U_DPAD);
				CheckDlgButton(statusDlg, IDC_CHECK_DLEFT, buttonDisplayed.L_DPAD);
				CheckDlgButton(statusDlg, IDC_CHECK_DRIGHT, buttonDisplayed.R_DPAD);
				CheckDlgButton(statusDlg, IDC_CHECK_DDOWN, buttonDisplayed.D_DPAD);
			}

			// begin accepting other messages
			initialized = true;

			// initial x/y text field values
			sprintf(str, "%d", initialStickX);
			SetDlgItemText(statusDlg, IDC_EDITX, str);
			sprintf(str, "%d", -initialStickY);
			SetDlgItemText(statusDlg, IDC_EDITY, str);

			// start WM_TIMER events going
			SetTimer(statusDlg, IDT_TIMER3, 50, (TIMERPROC) NULL);

			//Load combos
			//I realised there's HasPanel() too, but doesn't make much difference
			lBox = GetDlgItem(statusDlg, IDC_MACROLIST);
			if (lBox)
			{
					FreeCombos();
					InitialiseCombos(lBox);
			}
			// switch to emulator
			if(IsWindowFromEmulatorProcessActive())
				ActivateEmulatorWindow();

		}	break;

		case WM_ACTIVATE:
		case WM_SETFOCUS:
		{
			if(IsWindowFromEmulatorProcessActive())
				ActivateEmulatorWindow();
		}	break;
        case WM_NCDESTROY:
        case WM_DESTROY:
		{
			initialized = false;
//			buttonOverride.Value = 0;
//			buttonAutofire.Value = 0;
//			overrideOn = false;
			KillTimer(statusDlg, IDT_TIMER3); // XXX: what about the other windows' timers? We don't necessarily want to kill them. But this... doesn't work anyway?
			statusDlg = NULL;
		}	break;

		// too bad we don't get useful events like WM_MOUSEMOVE or WM_LBUTTONDOWN...
		case WM_SETCURSOR:
			nextClick = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) || (GetAsyncKeyState(VK_RBUTTON) & 0x8000));
			lastWasRight = 0!=(GetAsyncKeyState(VK_RBUTTON) & 0x8000);
			if(!dragging && !lastClick && nextClick)
			{
				if(IsMouseOverControl(statusDlg,IDC_STICKPIC))
				{
					if(draggingPermaStick || GetAsyncKeyState(VK_RBUTTON) & 0x8000)
					{
						draggingPermaStick = !draggingPermaStick;
						draggingStick = draggingPermaStick;
					}
					else
						draggingStick = true;
				}
				else if((!HasPanel(1) || // Any non-control area of the window is draggable.
					     (!IsMouseOverControl(statusDlg,IDC_XABS) && // This is done partly out of necessity,
				          !IsMouseOverControl(statusDlg,IDC_XSEM) && // because normal automatic window movement
				          !IsMouseOverControl(statusDlg,IDC_XREL) && // would not work (don't get messages for it),
				          !IsMouseOverControl(statusDlg,IDC_XRAD) && // and partly because the result occupies
				          !IsMouseOverControl(statusDlg,IDC_YABS) && // less space and is more convenient
				          !IsMouseOverControl(statusDlg,IDC_YSEM) &&
				          !IsMouseOverControl(statusDlg,IDC_YREL) &&
				          !IsMouseOverControl(statusDlg,IDC_EDITX) &&
				          !IsMouseOverControl(statusDlg,IDC_EDITY) &&
				          !IsMouseOverControl(statusDlg,IDC_SPINX) &&
				          !IsMouseOverControl(statusDlg,IDC_SPINY) &&
				          !IsMouseOverControl(statusDlg,IDC_SLIDERX) &&
				          !IsMouseOverControl(statusDlg,IDC_SLIDERY) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON0) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON1))) &&
					    (!HasPanel(2) ||
					     (!IsMouseOverControl(statusDlg,IDC_CLEARBUTTONS) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON2) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON3) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON4))) &&
					    (!HasPanel(3) ||
					     (!IsMouseOverControl(statusDlg,IDC_MACROBOX) &&
				          !IsMouseOverControl(statusDlg,IDC_MACROLIST) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON5) &&
				          !IsMouseOverControl(statusDlg,IDC_MOREBUTTON6))))
				{
					if(HasPanel(2) &&
					   (IsMouseOverControl(statusDlg,IDC_CHECK_A) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_B) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_L) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_R) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_Z) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_START) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_CUP) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_CDOWN) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_CLEFT) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_CRIGHT) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_DUP) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_DDOWN) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_DLEFT) ||
				        IsMouseOverControl(statusDlg,IDC_CHECK_DRIGHT)))
					{
						if(GetAsyncKeyState(VK_RBUTTON) & 0x8000) // right click on a button to autofire it
						{
#define UPDATEAUTO(idc,field) \
{ \
	if(IsMouseOverControl(statusDlg,idc)) \
	{ \
		BUTTONS &autoFire1 = (frameCounter%2 == 0) ? buttonAutofire : buttonAutofire2; \
		BUTTONS &autoFire2 = (frameCounter%2 == 0) ? buttonAutofire2 : buttonAutofire; \
		autoFire1.field = !(autoFire1.field|autoFire2.field); \
		autoFire2.field = 0; \
		buttonOverride.field = 0; \
	} \
}
							UPDATEAUTO(IDC_CHECK_A, A_BUTTON);
							UPDATEAUTO(IDC_CHECK_B, B_BUTTON);
							UPDATEAUTO(IDC_CHECK_START, START_BUTTON);
							UPDATEAUTO(IDC_CHECK_L, L_TRIG);
							UPDATEAUTO(IDC_CHECK_R, R_TRIG);
							UPDATEAUTO(IDC_CHECK_Z, Z_TRIG);
							UPDATEAUTO(IDC_CHECK_CUP, U_CBUTTON);
							UPDATEAUTO(IDC_CHECK_CLEFT, L_CBUTTON);
							UPDATEAUTO(IDC_CHECK_CRIGHT, R_CBUTTON);
							UPDATEAUTO(IDC_CHECK_CDOWN, D_CBUTTON);
							UPDATEAUTO(IDC_CHECK_DUP, U_DPAD);
							UPDATEAUTO(IDC_CHECK_DLEFT, L_DPAD);
							UPDATEAUTO(IDC_CHECK_DRIGHT, R_DPAD);
							UPDATEAUTO(IDC_CHECK_DDOWN, D_DPAD);
#undef UPDATEAUTO
							ActivateEmulatorWindow();
						}
					}
					else
					{
						dragging = true;
						POINT pt;
						GetCursorPos(&pt);
						dragXStart = pt.x;
						dragYStart = pt.y;

						RECT rect;
						GetWindowRect(statusDlg, &rect);
						dragXStart -= rect.left;
						dragYStart -= rect.top;
					}
				}
			}
			lastClick = nextClick;
			/* fall through */
		case WM_NCHITTEST:
		case WM_TIMER:
			skipEditX = false;
			skipEditY = false;
			if(resetXScale)
				resetXScale = false, SendDlgItemMessage(statusDlg, IDC_SLIDERX, TBM_SETPOS, TRUE, (LPARAM)(LONG)(1000));
			if(resetYScale)
				resetYScale = false, SendDlgItemMessage(statusDlg, IDC_SLIDERY, TBM_SETPOS, TRUE, (LPARAM)(LONG)(1000));
			if(dragging && !nextClick)
			{
				dragging = false;
				ActivateEmulatorWindow();
			}
			if(draggingStick && ((!nextClick && !draggingPermaStick ) || !IsWindowFromEmulatorProcessActive()))
			{
				draggingStick = false;
				draggingPermaStick = false;
				if(IsWindowFromEmulatorProcessActive())
					ActivateEmulatorWindow();
			}
			if(dragging)
			{
				POINT pt;
				GetCursorPos(&pt);
				int newDragX = pt.x;
				int newDragY = pt.y;
				if(lastXDrag != newDragX-dragXStart || lastYDrag != newDragY-dragYStart)
				{
					lastXDrag = newDragX-dragXStart;
					lastYDrag = newDragY-dragYStart;
					SetWindowPos(statusDlg,0, lastXDrag, lastYDrag, 0,0, SWP_NOSIZE|SWP_SHOWWINDOW);
				}
			}
			else if(draggingStick)
			{
				bool changed = false;
				int oldOverrideX = overrideOn ? overrideX : LastControllerInput.X_AXIS;
				int oldOverrideY = overrideOn ? overrideY : LastControllerInput.Y_AXIS;
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(GetDlgItem(statusDlg, IDC_STICKPIC), &pt);
				overrideX =  (pt.x*256/STICKPIC_SIZE - 128 + 1);
				overrideY = -(pt.y*256/STICKPIC_SIZE - 128 + 1);
				
				// normalize out-of-bounds clicks
				if(overrideX > 127 || overrideY > 127 || overrideX < -128 || overrideY < -129)
				{
					int absX = abs(overrideX);
					int absY = abs(overrideY);
					int div = (absX > absY) ? absX : absY;
					overrideX = overrideX * (overrideX > 0 ? 127 : 128) / div;
					overrideY = overrideY * (overrideY > 0 ? 127 : 128) / div;
				}
				if(overrideX < 7 && overrideX > -7) // snap near-zero clicks to zero
					overrideX = 0;
				if(overrideY < 7 && overrideY > -7)
					overrideY = 0;

				if(oldOverrideX != overrideX || (AngDisp && oldOverrideY != overrideY))
				{
					char str [256];
					if(!AngDisp)
						sprintf(str, "%d", overrideX);
					else
					{
						float radialAngle = 4*PI + atan2f((float)-overrideY, (float)overrideX);
						float angle2 = fmodf(90.0f + radialAngle*(180.0f/PI), 360.0f);
						sprintf(str, "%d", (int)(angle2 + (angle2>0 ? 0.5f : -0.5f)));
						skipEditX = true;
					}
					SetDlgItemText(statusDlg, IDC_EDITX, str);
					changed = true;
				}
				if(oldOverrideY != overrideY || (AngDisp && oldOverrideX != overrideX))
				{
					char str [256];
					if(!AngDisp)
						sprintf(str, "%d", -overrideY);
					else
					{
						float radialDistance = sqrtf((float)(overrideX*overrideX + overrideY*overrideY));
						sprintf(str, "%d", (int)(0.5f + radialDistance));
						skipEditY = true;
					}
					SetDlgItemText(statusDlg, IDC_EDITY, str);
					changed = true;
				}

				if(changed)
				{
					radialRecalc = true;
					overrideOn = true;
					RefreshAnalogPicture();
				}
				ActivateEmulatorWindow();
			}
			else if(IsWindowFromEmulatorProcessActive())
			{
				if(!IsAnyStatusDialogActive())
				{
					if(gettingKeys)
						Sleep(0);
					ActivateEmulatorWindow();
					if(!gettingKeys)
					{
						BUTTONS Keys;
						relativeControlNow = (msg == WM_TIMER);
//						incrementingFrameNow = false;
						//GetKeys(&Keys);      will it break anything?? it was messing with combo counter
//						incrementingFrameNow = true;
						relativeControlNow = false;
						ActivateEmulatorWindow();
					}
				}
				else
				{
					if(((GetKeyState(VK_ESCAPE) & 0x8000) || (GetKeyState(VK_RETURN) & 0x8000) || (deactivateAfterClick && !(GetAsyncKeyState(VK_LBUTTON) & 0x8000))))
					{
						ActivateEmulatorWindow();
						deactivateAfterClick = false;
					}
				}
				SetWindowPos(statusDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
			}
			break;
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdcmain = BeginPaint(statusDlg, &ps);
				HDC hdc = CreateCompatibleDC(hdcmain); //buffer 
				RECT rect, rect2;
				GetWindowRect(GetDlgItem(statusDlg,IDC_STICKPIC), &rect);
				GetWindowRect(statusDlg, &rect2);
				//move to relative of whole window, adjusted because ellipse sucks and because bottom right is exclusive
				rect.left -= rect2.left+2; rect.right  -= rect2.left+4;
				rect.top  -= rect2.top+2;  rect.bottom -= rect2.top+4;

				int w = rect.right;
				int h = rect.bottom;
				HBITMAP hBmp = CreateCompatibleBitmap(hdcmain, w, h);   //!!! it has to be compatible with  main dc, not buffer!
				SelectObject(hdc, hBmp); //use the bmp to draw

				FillRect(hdc, &rect, GetSysColorBrush(COLOR_BTNFACE)); //I clear background on my own
				Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom);

				HPEN hpenOld, hpenBlue, hpenRed;
				hpenBlue = CreatePen(PS_SOLID, 3, RGB(0, 0, 255)); // these need to be re-created every time...
				hpenRed = CreatePen(PS_SOLID, 7, RGB(255, 0, 0));
				hpenOld = (HPEN)SelectObject(hdc, hpenBlue);

				MoveToEx(hdc, (rect.left+rect.right)>>1, (rect.top+rect.bottom)>>1, NULL);
				int x = rect.left + ( overrideX+128)*(rect.right-rect.left)/256;
				int y = rect.top  + (-overrideY+128)*(rect.bottom-rect.top)/256;
				LineTo(hdc, x,y);

				SelectObject(hdc, hpenOld);
				DeleteObject(hpenBlue);

				MoveToEx(hdc, rect.left, (rect.top+rect.bottom)>>1, NULL);
				LineTo(hdc, rect.right, (rect.top+rect.bottom)>>1);
				MoveToEx(hdc, (rect.left+rect.right)>>1, rect.top, NULL);
				LineTo(hdc, (rect.left+rect.right)>>1, rect.bottom);

				hpenOld = (HPEN)SelectObject(hdc, hpenRed);
				MoveToEx(hdc, x,y, NULL);
				LineTo(hdc, x,y);
				SelectObject(hdc, hpenOld);
				DeleteObject(hpenRed);
				BitBlt(hdcmain, rect.left, rect.top, w, h, hdc, rect.left, rect.top, SRCCOPY);
				EndPaint (statusDlg, &ps) ;
				//free everything to avoid memory leak
				DeleteDC(hdc);
				DeleteDC(hdcmain);
				DeleteObject(hBmp);
				DeleteObject(hpenOld);

			}

			break;

//		case UDN_DELTAPOS:
//			break;
		case WM_NOTIFY:
		{
			switch(LOWORD(wParam))
			{
				case IDC_SPINX:
				case IDC_SPINY:
				{
					if(GetAsyncKeyState(VK_LBUTTON) & 0x8000)
						deactivateAfterClick = true;
				}	break;

				case IDC_SLIDERX:
				{
					if(lastWasRight || GetAsyncKeyState(VK_RBUTTON) & 0x8000)
					{
						resetXScale = true; // right-click resets to 100% scale (defer setpos until later because it would ruin the the control to do it during a WM_NOTIFY)
						deactivateAfterClick = true;
					}
					int pos = SendDlgItemMessage(statusDlg, IDC_SLIDERX, TBM_GETPOS, 0, 0);
					if(GetAsyncKeyState(VK_LBUTTON) & 0x8000)
						deactivateAfterClick = true;
				}	break;

				case IDC_SLIDERY:
				{
					if(lastWasRight || GetAsyncKeyState(VK_RBUTTON) & 0x8000)
					{
						resetYScale = true;
						deactivateAfterClick = true;
					}
					int pos = SendDlgItemMessage(statusDlg, IDC_SLIDERY, TBM_GETPOS, 0, 0);
					if(GetAsyncKeyState(VK_LBUTTON) & 0x8000)
						deactivateAfterClick = true;
				}	break;
			}
		}	break;

		case EDIT_END:
			EndEdit(activeCombo,(char*)lParam);
			break;
		case WM_COMMAND:
			switch (LOWORD(wParam)) 
            {
				case IDC_EDITX:
				if(!skipEditX)
				{
					bool changed = false;
					char str [256];
					int newOverrideX=overrideX;
					GetDlgItemText(statusDlg, IDC_EDITX, str, 256);
					if(!AngDisp)
					{
						sscanf(str, "%d", &newOverrideX);
						if(newOverrideX > 127 || newOverrideX < -128)
						{
							if(newOverrideX > 127) newOverrideX = 127;
							if(newOverrideX < -128) newOverrideX = -128;
							sprintf(str, "%d", newOverrideX);
							SetDlgItemText(statusDlg, IDC_EDITX, str);
						}
					}
					else
					{
						int newAng;
						if(sscanf(str, "%d", &newAng))
						{
							if(newAng >= 360)
							{
								sprintf(str, "%d", newAng-360);
								SetDlgItemText(statusDlg, IDC_EDITX, str);
							}
							else if(newAng < 0)
							{
								sprintf(str, "%d", newAng+360);
								SetDlgItemText(statusDlg, IDC_EDITX, str);
							}
							float newAngF = (newAng - 90) * (PI/180.0f);
							newOverrideX =  (int)(xScale * radialDistance * cosf((float)newAngF));
							overrideY = -(int)(yScale * radialDistance * sinf((float)newAngF));
							if(newOverrideX > 127) newOverrideX = 127;
							if(newOverrideX < -128) newOverrideX = -128;
							if(overrideY > 127) overrideY = 127;
							if(overrideY < -128) overrideY = -128;
						}
					}
					if(overrideX != newOverrideX)
					{
						changed = true;
						overrideX = newOverrideX;
					}
					if(changed)
					{
						overrideOn = true;
						RefreshAnalogPicture ();
					}
				}	break;

				case IDC_EDITY:
				if(!skipEditY)
				{
					bool changed = false;
					char str [256];
					int newOverrideY=overrideY;
					GetDlgItemText(statusDlg, IDC_EDITY, str, 256);
					if(!AngDisp)
					{
						sscanf(str, "%d", &newOverrideY);
						newOverrideY = -newOverrideY;
						if(newOverrideY > 127 || newOverrideY < -128)
						{
							if(newOverrideY > 127) newOverrideY = 127;
							if(newOverrideY < -128) newOverrideY = -128;
							sprintf(str, "%d", -newOverrideY);
							SetDlgItemText(statusDlg, IDC_EDITY, str);
						}
					}
					else
					{
						int newDist = (int)radialDistance;
						sscanf(str, "%d", &newDist);
						   overrideX =  (int)(xScale * (float)newDist * cosf(radialAngle));
						newOverrideY = -(int)(yScale * (float)newDist * sinf(radialAngle));
						if(newOverrideY > 127) newOverrideY = 127;
						if(newOverrideY < -128) newOverrideY = -128;
						if(overrideX > 127) overrideX = 127;
						if(overrideX < -128) overrideX = -128;
					}
					if(overrideY != newOverrideY)
					{
						changed = true;
						overrideY = newOverrideY;
					}
					if(changed)
					{
						overrideOn = true;
						RefreshAnalogPicture ();
					}
				}	break;

				case IDC_XREL:
				case IDC_XSEM:
				case IDC_XABS:
				case IDC_XRAD:
				case IDC_YREL:
				case IDC_YSEM:
				case IDC_YABS:
				{
					if(IsDlgButtonChecked(statusDlg, IDC_XABS)) relativeXOn = 0;
					if(IsDlgButtonChecked(statusDlg, IDC_YABS)) relativeYOn = 0;
					if(IsDlgButtonChecked(statusDlg, IDC_XSEM)) relativeXOn = 1;
					if(IsDlgButtonChecked(statusDlg, IDC_YSEM)) relativeYOn = 1;
					if(IsDlgButtonChecked(statusDlg, IDC_XREL)) relativeXOn = 2;
					if(IsDlgButtonChecked(statusDlg, IDC_YREL)) relativeYOn = 2;
					if(IsDlgButtonChecked(statusDlg, IDC_XRAD)) relativeXOn = 3;
					radialRecalc = true;

					HKEY hKey;
					DWORD dwDWType = REG_DWORD;
					DWORD dwDWSize = sizeof(DWORD);			
					if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS ) {
						char str [256];
						sprintf(str, "Controller%dRelativeX", Control);
						RegSetValueEx(hKey, str, 0, dwDWType, (LPBYTE)&relativeXOn, dwDWSize);
						sprintf(str, "Controller%dRelativeY", Control);
						RegSetValueEx(hKey, str, 0, dwDWType, (LPBYTE)&relativeYOn, dwDWSize);
					}
					RegCloseKey(hKey);

					ActivateEmulatorWindow();
				}	break;

				case IDC_CHECK_ANGDISP:
				{
					if(IsDlgButtonChecked(statusDlg, IDC_CHECK_ANGDISP))
					{
						AngDisp = true;
						SetDlgItemText(statusDlg, IDC_STATICX, "a");
						SetDlgItemText(statusDlg, IDC_STATICY, "D");
						SendDlgItemMessage(statusDlg, IDC_SPINX, UDM_SETRANGE, 0, (LPARAM)MAKELONG(720,-720));
						SendDlgItemMessage(statusDlg, IDC_SPINY, UDM_SETRANGE, 0, (LPARAM)MAKELONG(180,-180));
					}
					else
					{
						AngDisp = false;
						SetDlgItemText(statusDlg, IDC_STATICX, "X");
						SetDlgItemText(statusDlg, IDC_STATICY, "Y");
						SendDlgItemMessage(statusDlg, IDC_SPINX, UDM_SETRANGE, 0, (LPARAM)MAKELONG(128,-127));
						SendDlgItemMessage(statusDlg, IDC_SPINY, UDM_SETRANGE, 0, (LPARAM)MAKELONG(-127,128));
					}

					HKEY hKey;
					DWORD dwDWType = REG_DWORD;
					DWORD dwDWSize = sizeof(DWORD);			
					if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS ) {
						char str [256];
						sprintf(str, "Controller%dAngDisp", Control);
						RegSetValueEx(hKey, str, 0, dwDWType, (LPBYTE)&AngDisp, dwDWSize);
					}
					RegCloseKey(hKey);

					ActivateEmulatorWindow();
				}	break;

				case IDC_CHECK_A:      buttonOverride.A_BUTTON =     IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.A_BUTTON =     buttonAutofire2.A_BUTTON =     0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_B:      buttonOverride.B_BUTTON =     IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.B_BUTTON =     buttonAutofire2.B_BUTTON =     0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_START:  buttonOverride.START_BUTTON = IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.START_BUTTON = buttonAutofire2.START_BUTTON = 0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_Z:      buttonOverride.Z_TRIG =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.Z_TRIG =       buttonAutofire2.Z_TRIG =       0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_L:      buttonOverride.L_TRIG =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.L_TRIG =       buttonAutofire2.L_TRIG =       0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_R:      buttonOverride.R_TRIG =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.R_TRIG =       buttonAutofire2.R_TRIG =       0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_CLEFT:  buttonOverride.L_CBUTTON =    IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.L_CBUTTON =    buttonAutofire2.L_CBUTTON =    0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_CUP:    buttonOverride.U_CBUTTON =    IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.U_CBUTTON =    buttonAutofire2.U_CBUTTON =    0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_CRIGHT: buttonOverride.R_CBUTTON =    IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.R_CBUTTON =    buttonAutofire2.R_CBUTTON =    0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_CDOWN:  buttonOverride.D_CBUTTON =    IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.D_CBUTTON =    buttonAutofire2.D_CBUTTON =    0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_DLEFT:  buttonOverride.L_DPAD =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.L_DPAD =       buttonAutofire2.L_DPAD =       0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_DUP:    buttonOverride.U_DPAD =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.U_DPAD =       buttonAutofire2.U_DPAD =       0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_DRIGHT: buttonOverride.R_DPAD =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.R_DPAD =       buttonAutofire2.R_DPAD =       0; ActivateEmulatorWindow(); break;
				case IDC_CHECK_DDOWN:  buttonOverride.D_DPAD =       IsDlgButtonChecked(statusDlg, LOWORD(wParam))?1:0; buttonAutofire.D_DPAD =       buttonAutofire2.D_DPAD =       0; ActivateEmulatorWindow(); break;
				case IDC_CLEARBUTTONS: buttonOverride.Value = 0; buttonAutofire.Value = 0; buttonAutofire2.Value = 0; ActivateEmulatorWindow(); break;

				case IDC_MOREBUTTON0:
				case IDC_MOREBUTTON1:
				case IDC_MOREBUTTON2:
				case IDC_MOREBUTTON3:
				case IDC_MOREBUTTON4:
				case IDC_MOREBUTTON5:
				case IDC_MOREBUTTON6:
				{
					switch(LOWORD(wParam)) // Extend
					{
			            case IDC_MOREBUTTON0: RemovePanel(1); break;
			   default: case IDC_MOREBUTTON1: AddPanel(2); break;
						case IDC_MOREBUTTON2: AddPanel(1); break;
						case IDC_MOREBUTTON3: RemovePanel(2); break;
						case IDC_MOREBUTTON4: AddPanel(3); break;
						case IDC_MOREBUTTON5: RemovePanel(3); break;
						case IDC_MOREBUTTON6: AddPanel(2); break;
					}

					comboTask = C_IDLE;
					RECT rect;
					GetWindowRect(statusDlg, &rect);
					xPosition = rect.left;
					yPosition = rect.top;
					positioned = true;

					HKEY hKey;
					DWORD dwDWType = REG_DWORD;
					DWORD dwDWSize = sizeof(DWORD);			
					if (RegOpenKeyEx(HKEY_CURRENT_USER, SUBKEY, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS ) {
						char str [256];
						sprintf(str, "Controller%dDlgExtend", Extend);
						RegSetValueEx(hKey, str, 0, dwDWType, (LPBYTE)&relativeXOn, dwDWSize);
					}
					RegCloseKey(hKey);

					// Extend the dialog by replacing it with a new one created from a different resource.
					// Resizing wouldn't work, because any resizing causes visible damage to the dialog's background
					// due to some messages not getting through to repair it
					StartThread(Control);
				}	break;
				case IDC_PLAY:
					activeCombo = ListBox_GetCurSel(GetDlgItem(statusDlg, IDC_MACROLIST));
					if (activeCombo == -1) break;
					comboTask = C_RUNNING;
					comboStart = frameCounter;
					break;
				case IDC_STOP:
					if (comboTask == C_RECORD) StopRecording();
					comboTask = C_IDLE;
					comboStart = 0; //should avoid unnecessary bugs
					break;
				case IDC_PAUSE:
					comboTask = C_PAUSE;
					break;
				case IDC_LOOP:
					if (comboTask == C_LOOP)
					{
						comboTask = C_RUNNING;
						break;
					}
					comboStart = frameCounter;
					comboTask = C_LOOP;
					break;
				case IDC_RECORD:
					if (comboTask == C_RECORD)
					{
						comboTask = C_IDLE;
						StopRecording();
						//free(aCombo.data); //is it better to copy to new, aligned buffer or leave it?
					}
					else
					{
						if (comboTask == C_IDLE || activeCombo==-1)
						{
							activeCombo = CreateNewCombo(-1);
							ListBox_SetCurSel(lBox, activeCombo);
							comboStart = frameCounter;
						}
						else
						{
							aCombo.length = frameCounter - comboStart;
							aCombo.data = ExtendBuffer(aCombo.data, aCombo.length*4);
						}
						comboTask = C_RECORD;
					}
					break;
				case IDC_EDIT:
					activeCombo = ListBox_GetCurSel(GetDlgItem(statusDlg, IDC_MACROLIST));
					if (activeCombo == -1) break;
					StartEdit(activeCombo);
				case IDC_CLEAR:
					//good joke, imagine msgbox working
					//if (MessageBox(0, "Do you want to remove everything?", "Warning", MB_YESNO| MB_ICONQUESTION) == 6)
					//{
					FreeCombos();
					ListBox_ResetContent(lBox);
					//}
					break;
				default:
//					SetFocus(statusDlg);
					break;
            }
			break;

		default:
			break;
	}
	return FALSE; //Using DefWindowProc is prohibited but worked anyway
}
