#include <stdbool.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdlib.h>

typedef struct {
	int X;
	int Y;
	int Width;
	int Height;
} entity;

struct StringArray {
	char *string;
	int length;
	int allocated;
};

typedef struct {
	int topX;
	int topY;
	int bottomX;
	int bottomY;
	char* InsertionPoint;
} caret;

struct SelectedArray {
	char* End1;
	char* End2;
	int Difference;
	int LeftX;
	int RightX;
	int TopY;
	int BottomY;
};

struct StringArray *array_constructor()
{
	struct StringArray *MyArray = (struct StringArray *) malloc(sizeof(struct StringArray));
	MyArray->length = 0;
	MyArray->allocated = 100;
	MyArray->string = malloc(100);
	MyArray->string[0] = '\0';
	return MyArray;
}

void draw_document(struct StringArray* PointerArray[], Display* MainDisplay, Window MainWindow, GC Context, caret MyCaret, XRectangle* Rectangles, int nRectangles)
{
	XClearWindow(MainDisplay, MainWindow);
	int CoordinateX = 10;
	int CoordinateY = 20;
	for(int i=0;PointerArray[i]!=NULL;++i){
		XDrawString(MainDisplay, MainWindow, Context, CoordinateX, CoordinateY, PointerArray[i]->string, PointerArray[i]->length);
		CoordinateY += 15;
	}
	XDrawLine(MainDisplay, MainWindow, Context, MyCaret.topX, MyCaret.topY, MyCaret.bottomX, MyCaret.bottomY);
	XDrawRectangles(MainDisplay, MainWindow, Context, Rectangles, nRectangles);
}


void expand_array(struct StringArray **MyArray, int *AllocatedLines)
{
	*AllocatedLines += 10;
	*MyArray = realloc(*MyArray, *AllocatedLines * sizeof(struct StringArray*));
}

int main() 
{
	Display* MainDisplay = XOpenDisplay(0);
	Window RootWindow = XDefaultRootWindow(MainDisplay);

	int DefaultScreen = DefaultScreen(MainDisplay);
	GC Context = XDefaultGC(MainDisplay, DefaultScreen);

	int WindowX = 0;
	int WindowY = 0;
	int WindowWidth = 800;
	int WindowHeight = 600;
	int BorderWidth = 0;
	int WindowDepth = CopyFromParent;
	int WindowClass = CopyFromParent;
	Visual* WindowVisual = CopyFromParent;

	int AttributeValueMask = CWBackPixel | CWEventMask;
	XSetWindowAttributes WindowAttributes = {};
	WindowAttributes.background_pixel = 0xffd7d0da;
	WindowAttributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask;

	Window MainWindow = XCreateWindow(MainDisplay, RootWindow, WindowX, WindowY, WindowWidth, WindowHeight, BorderWidth,
				   WindowDepth, WindowClass, WindowVisual, AttributeValueMask, &WindowAttributes);

	XMapWindow(MainDisplay, MainWindow);

	XStoreName(MainDisplay, MainWindow, "Moving rectangle, Use arrow keys to move.");

	Atom WM_DELETE_WINDOW = XInternAtom(MainDisplay, "WM_DELETE_WINDOW", False);
	if(!XSetWMProtocols(MainDisplay, MainWindow, &WM_DELETE_WINDOW, 1)) {
		printf("Couldn't register WM_DELETE_WINDOW property \n");
	}

	XIM StringInput = XOpenIM(MainDisplay, NULL, NULL, NULL);
	XIC StringContext = XCreateIC(StringInput, 
			       XNInputStyle, XIMPreeditNothing | XIMStatusNothing, 
			       XNClientWindow, MainWindow,
			       NULL);
	struct StringArray** LinePointers = malloc(10 * sizeof(struct StringArray*));
	struct SelectedArray Selected;
	LinePointers[0] = array_constructor();
	LinePointers[1] = NULL;
	int nLinePointers = 1;
	int AllocatedLines = 10;
	int CurrentArray = 0;
	int IsWindowOpen = 1;
	char buffer_return[1000];
	int bytes_buffer;
	KeySym keysym_return;
	Status status_return;
	int Test;
	bool TruePosition = false;
	bool ButtonPressed = false;
	ptrdiff_t position = 0;
	caret MyCaret = {10, 10, 10, 20, LinePointers[0]->string};
	XRectangle *Rectangles = (XRectangle *) malloc(100 * sizeof(XRectangle));
	int nRectangles = 0;
	int CurrentSelectedArray = 0;
	int Direction;

/*Continue working on select functionality. Do this by refactoring the down selecting and when selecting in either direction it regresses a line. Same thing that happens when going left then right for selecting*/

	while(IsWindowOpen) {
		XEvent GeneralEvent = {};
		XNextEvent(MainDisplay, &GeneralEvent);
		/*Button Press*/
		if(GeneralEvent.type == ButtonPress){
			position = false;
			XButtonEvent *Event = &GeneralEvent.xbutton;
			int CoordX = Event->x;
			int CoordY = Event->y;
			int XPosition = (CoordX-10)/6;
			int YPosition = (CoordY-10)/15;
			if(YPosition <= nLinePointers-1){
				CurrentArray = YPosition;
				CurrentSelectedArray = CurrentArray;
				if(XPosition <= LinePointers[YPosition]->length){
					MyCaret.topX = (XPosition*6)+10;
					MyCaret.bottomX = (XPosition*6)+10;
					MyCaret.topY = (YPosition*15)+10;
					MyCaret.bottomY = (YPosition*15)+20;
					MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
					CurrentSelectedArray = CurrentArray;
				}
			}
		}
		/*Motion Notify or when the mouse moves*/
		else if(GeneralEvent.type == MotionNotify){
			position = false;
			XPointerMovedEvent *Event = &GeneralEvent.xmotion;
			if(Event->state == Button1Mask){
				int CoordX = Event->x;
				int CoordY = Event->y;
				int XPosition = (CoordX-10)/6;
				int YPosition = (CoordY-10)/15;
			if(YPosition <= nLinePointers-1){
				if(XPosition <= LinePointers[YPosition]->length){
					if(ButtonPressed==false){
						Selected.End1 = MyCaret.InsertionPoint;
						Selected.End2 = MyCaret.InsertionPoint;
						Selected.Difference = 0;
						Selected.LeftX = Selected.RightX = MyCaret.topX;
						Selected.TopY = MyCaret.topY;
						Selected.BottomY = MyCaret.bottomY;
						ButtonPressed = true;
					}
					/*Going outside the leftmost bound on the same line as the insertion point*/
					if(CoordX < Selected.LeftX){
						Direction = 0;
						--Selected.End2;
						Selected.LeftX -= 6;
						MyCaret.topX = ((XPosition)*6)+10;
						MyCaret.bottomX = ((XPosition)*6)+10;
						MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
						CurrentArray = YPosition;
						if(nRectangles == Selected.Difference){
							XRectangle Rectangle = {Selected.LeftX, Selected.TopY, (Selected.RightX-Selected.LeftX), 10};
							Rectangles[nRectangles] = Rectangle;
							++nRectangles;
						}
						else if (nRectangles==1){
							Rectangles[nRectangles-1].x = Selected.LeftX;
							Rectangles[nRectangles-1].width = Selected.RightX - Selected.LeftX;
						}
						else{
							Rectangles[nRectangles-1].x = Selected.LeftX;
							Rectangles[nRectangles-1].width = ((LinePointers[CurrentArray]->length)-XPosition)*6;
						}
					}
					/*Going outside the rightmost bound on the same line as the insertion point*/
					if(CoordX > Selected.RightX){
						Direction = 1;
						++Selected.End2;
						Selected.RightX += 6;
						MyCaret.topX = ((XPosition+1)*6)+10;
						MyCaret.bottomX = ((XPosition+1)*6)+10;
						MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
						CurrentArray = YPosition;
						if(nRectangles == Selected.Difference){
							XRectangle Rectangle = {Selected.LeftX, Selected.TopY, (Selected.RightX-Selected.LeftX), 10};
							Rectangles[nRectangles] = Rectangle;
							++nRectangles;
						}
						else{
							Rectangles[nRectangles-1].width = Selected.RightX - Selected.LeftX;
						}
					}
					/*Move the highlighted area in from the leftmost bound on the line of the insertion point*/
					if(CoordX > Selected.LeftX+6 && Direction == 0){
						--Selected.End2;
						Selected.LeftX += 6;
						MyCaret.topX = ((XPosition)*6)+10;
						MyCaret.bottomX = ((XPosition)*6)+10;
						MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
						CurrentArray = YPosition;
						Rectangles[nRectangles-1].x += 6;
						Rectangles[nRectangles-1].width -= 6;
					}
					/*Move the highlighted area in from the rightmost bound on the line of the insertion point*/
					if(CoordX < Selected.RightX-6 && Direction == 1){
						++Selected.End2;
						Selected.RightX -= 6;
						MyCaret.topX = ((XPosition+1)*6)+10;
						MyCaret.bottomX = ((XPosition+1)*6)+10;
						MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
						CurrentArray = YPosition;
						Rectangles[nRectangles-1].width -= 6;
					}
					/*Move highlighting down an array*/
					if(CoordY > Selected.BottomY){
							++Selected.Difference;
							if(Selected.Difference<2)
								position = Selected.End1 - LinePointers[CurrentSelectedArray]->string;
							else{
								position = 0;
								for(int i=1;i==nRectangles-1;++i){
									Rectangles[i].x = 10;
									Rectangles[i].width = (LinePointers[CurrentSelectedArray+i]->length)*6;
								}
							}
							Rectangles[nRectangles-1].width = ((LinePointers[CurrentSelectedArray]->length)-position)*6;
							XRectangle Rectangle = {10, (Selected.TopY+(15*nRectangles)), (Selected.RightX-10), 10};
							Rectangles[nRectangles] = Rectangle;
							++nRectangles;
							Selected.BottomY += 15;
							++CurrentSelectedArray;
					}
					/*Move highlighting up an array*/
					if(CoordY < Selected.TopY){
						++Selected.Difference;
						Direction = 0;
						MyCaret.topX = (XPosition*6)+10;
						MyCaret.bottomX = (XPosition*6)+10;
						MyCaret.topY = (YPosition*15)+10;
						MyCaret.bottomY = (YPosition*15)+20;
						MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
						CurrentArray = YPosition;
						if(nRectangles==0){
							position = Selected.End1 - LinePointers[CurrentSelectedArray]->string;
							/*++position;*/
							XRectangle Rectangle1 = {10, Selected.TopY, (position*6), 10};
							Rectangles[nRectangles] = Rectangle1;
							Selected.TopY -= 15;
							--CurrentSelectedArray;
							++nRectangles;
							int width = LinePointers[CurrentSelectedArray]->length - XPosition;
							XRectangle Rectangle2 = {MyCaret.topX, Selected.TopY,(width*6), 10};
							Selected.LeftX = MyCaret.topX;
							Selected.RightX = MyCaret.topX + (width*6);
							Rectangles[nRectangles] = Rectangle2;
							++nRectangles;
						}
						else if(nRectangles==1){
							position = Selected.End1 - LinePointers[CurrentSelectedArray]->string;
							Rectangles[0].x = 10;
							Rectangles[0].width = position*6;
							++position;
							Selected.TopY -= 15;
							--CurrentSelectedArray;
							int width = LinePointers[CurrentSelectedArray]->length - (XPosition);
							XRectangle Rectangle = {MyCaret.topX, Selected.TopY,(width*6), 10};
							Selected.LeftX = MyCaret.topX;
							Selected.RightX = MyCaret.topX + (width*6);
							Rectangles[nRectangles] = Rectangle;
							++nRectangles;
						}
						else{
							Selected.TopY -= 15;
							--CurrentSelectedArray;
							int width = LinePointers[CurrentSelectedArray]->length - XPosition;
							XRectangle Rectangle = {MyCaret.topX, Selected.TopY,(width*6), 10};
							Selected.LeftX = MyCaret.topX;
							Selected.RightX = MyCaret.topX + (width*6);
							Rectangles[nRectangles] = Rectangle;
							for(int i=1;i!=nRectangles;++i){
								Rectangles[i].x = 10;
								Rectangles[i].width = LinePointers[CurrentSelectedArray+(nRectangles-i)]->length * 6;
							}
							++nRectangles;
						}
					}
					if(CoordY > Selected.TopY + 15){
						--Selected.Difference;
						Direction = 0;
						MyCaret.topX = (XPosition*6)+10;
						MyCaret.bottomX = (XPosition*6)+10;
						MyCaret.topY = (YPosition*15)+10;
						MyCaret.bottomY = (YPosition*15)+20;
						MyCaret.InsertionPoint = &LinePointers[YPosition]->string[XPosition];
						CurrentArray = YPosition;
						CurrentSelectedArray=CurrentArray;
						if(nRectangles > 2){
							Rectangles[nRectangles-2].x = MyCaret.topX;
							Rectangles[nRectangles-2].y = MyCaret.topY;
							int width = LinePointers[CurrentSelectedArray]->length - XPosition;
							Rectangles[nRectangles-2].width = width*6 ;
							Selected.TopY = MyCaret.topY;
							Selected.LeftX = MyCaret.topX;
							Selected.RightX = MyCaret.topX + (width*6);
						}
						else{
							Selected.TopY = MyCaret.topY;
							Selected.LeftX = MyCaret.topX;
							Selected.RightX = Rectangles[0].x + Rectangles[0].width;
							if(MyCaret.topX > Rectangles[0].x){
								int width = (Rectangles[0].x + Rectangles[0].width)-MyCaret.topX;
								Rectangles[0].x = MyCaret.topX;
								Rectangles[0].width = width;
							}
							/*Needs to be corrected. This is what happens when you go down after going up on the right side.*/
							else {
								int width = LinePointers[CurrentSelectedArray]->length - XPosition;
								Rectangles[0].x = Rectangles[0].x + Rectangles[0].width;
								Rectangles[0].width = width*6;
							}
						}
						--nRectangles;
					}
				}
			}
			}
		}
		/*Button Released*/
		else if(GeneralEvent.type == ButtonRelease){
			nRectangles = 0;
			ButtonPressed = false;
		}
		else if(GeneralEvent.type == KeyPress){
			ButtonPressed = false;
			XKeyEvent *Event = &GeneralEvent.xkey;
			if(AllocatedLines <= nLinePointers+1) {
				expand_array(LinePointers, &AllocatedLines);
			}
			/*Backspace*/
			if(Event->keycode == 22){
				TruePosition = false;
				if((LinePointers[CurrentArray]->length)==0 && CurrentArray!=0){
					struct StringArray* Next = LinePointers[CurrentArray+1];
					for(int i=CurrentArray;LinePointers[i]!=NULL;++i){
						LinePointers[i] = Next;
						Next = LinePointers[i+1];
					}
					--CurrentArray;
					--nLinePointers;
					MyCaret.topX = ((LinePointers[CurrentArray]->length) * 6) + 16;
					MyCaret.bottomX = ((LinePointers[CurrentArray]->length) * 6) + 16;
					MyCaret.topY -= 15;
					MyCaret.bottomY -= 15;
					MyCaret.InsertionPoint = &LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length];
				}
				else if((MyCaret.InsertionPoint != &LinePointers[CurrentArray]->string[0])){
					--LinePointers[CurrentArray]->length;
					LinePointers[CurrentArray]->string[(LinePointers[CurrentArray]->length)+1] = '\0';
					MyCaret.topX -= 6;
					MyCaret.bottomX -= 6;
					--MyCaret.InsertionPoint;
				}
			}
			/*Tab*/
			else if(Event->keycode == 23){
				TruePosition = false;
				for(int i=4;i>0;--i){
					LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length] = ' ';
					LinePointers[CurrentArray]->string[(LinePointers[CurrentArray]->length)+1] = '\0';
					LinePointers[CurrentArray]->length++;
					MyCaret.topX += 6;
					MyCaret.bottomX += 6;
					++MyCaret.InsertionPoint;
				}
			}
			/*Return*/
			else if(Event->keycode == 36){
				TruePosition = false;
				if(LinePointers[CurrentArray+1]!=NULL){
					struct StringArray* Current = LinePointers[CurrentArray];
					LinePointers[CurrentArray] = array_constructor();
					++nLinePointers;
					struct StringArray* Previous;
					int i;
					for(i=CurrentArray+1;LinePointers[i]!=NULL;++i){
						Previous = Current;
						Current = LinePointers[i];
						LinePointers[i] = Previous;
					}
					LinePointers[i] = Current;
					LinePointers[i+1] = NULL;
					++CurrentArray;
					MyCaret.topX = 10;
					MyCaret.bottomX = 10;
					MyCaret.topY += 15;
					MyCaret.bottomY += 15;
					MyCaret.InsertionPoint = LinePointers[CurrentArray]->string;
				}
				else {
					++CurrentArray;
					LinePointers[CurrentArray] = array_constructor();
					LinePointers[CurrentArray+1] = NULL;
					++nLinePointers;
					MyCaret.topX = 10;
					MyCaret.bottomX = 10;
					MyCaret.topY += 15;
					MyCaret.bottomY += 15;
					MyCaret.InsertionPoint = LinePointers[CurrentArray]->string;
				}

			}
			/*Left*/
			else if(Event->keycode == 113){
				TruePosition = false;
				if(MyCaret.topX != 10){
					MyCaret.topX -= 6;
					MyCaret.bottomX -= 6;
					--MyCaret.InsertionPoint;
				}
			}
			/*Right*/
			else if(Event->keycode == 114){
				TruePosition = false;
				if(MyCaret.topX != ((LinePointers[CurrentArray]->length) * 6) + 10){
					MyCaret.topX += 6;
					MyCaret.bottomX += 6;
					++MyCaret.InsertionPoint;
				}
			}
			/*Up*/
			else if(Event->keycode == 111){
				if(MyCaret.topY != 10){
					MyCaret.topY -= 15;
					MyCaret.bottomY -= 15;
					if(TruePosition==false){
						position = MyCaret.InsertionPoint - LinePointers[CurrentArray]->string;
						TruePosition = true;
					}
					if(position>LinePointers[--CurrentArray]->length){
						MyCaret.InsertionPoint = &LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length];
						MyCaret.topX = ((LinePointers[CurrentArray]->length) * 6) + 10;
						MyCaret.bottomX = ((LinePointers[CurrentArray]->length) * 6) + 10;

					}
					else {
						MyCaret.InsertionPoint = &LinePointers[CurrentArray]->string[position];
						MyCaret.topX = (position * 6) + 10;
						MyCaret.bottomX = (position * 6) + 10;
					}
				}
			}
			/*Down*/
			else if(Event->keycode == 116){
				if(LinePointers[CurrentArray+1]!=NULL){
					MyCaret.topY += 15;
					MyCaret.bottomY += 15;
					if(TruePosition==false){
						position = MyCaret.InsertionPoint - LinePointers[CurrentArray]->string;
						TruePosition = true;
					}
					if(position>LinePointers[++CurrentArray]->length){
						MyCaret.InsertionPoint = &LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length];
						MyCaret.topX = ((LinePointers[CurrentArray]->length) * 6) + 10;
						MyCaret.bottomX = ((LinePointers[CurrentArray]->length) * 6) + 10;
					}
					else {
						MyCaret.InsertionPoint = &LinePointers[CurrentArray]->string[position];
						MyCaret.topX = (position * 6) + 10;
						MyCaret.bottomX = (position * 6) + 10;
					}
				}
			}
			/*Shift L and R*/
			else if(Event->keycode == 50 || Event->keycode == 62) ;
			/*Insert Character*/
			else {
				TruePosition = false;
				if(*MyCaret.InsertionPoint != '\0'){
					char Current = *MyCaret.InsertionPoint;
					char Previous;
					char * ipoint=(MyCaret.InsertionPoint)+1;
					for(ipoint=ipoint;*ipoint!='\0';++ipoint){
						Previous=Current;
						Current = *ipoint;
						*ipoint = Previous;
					}
					*ipoint = Current;
					++ipoint;
					ipoint = '\0';
					++LinePointers[CurrentArray]->length;
					bytes_buffer = sizeof(buffer_return);
					Test = XmbLookupString(StringContext, Event, buffer_return, bytes_buffer, &keysym_return, &status_return);
					*MyCaret.InsertionPoint = *buffer_return;
					++MyCaret.InsertionPoint;
					MyCaret.topX += 6;
					MyCaret.bottomX += 6;
				}
				else {
					bytes_buffer = sizeof(buffer_return);
					Test = XmbLookupString(StringContext, Event, buffer_return, bytes_buffer, &keysym_return, &status_return);
					LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length] = *buffer_return;
					LinePointers[CurrentArray]->string[(LinePointers[CurrentArray]->length)+1] = '\0';
					++LinePointers[CurrentArray]->length;
					MyCaret.topX += 6;
					MyCaret.bottomX += 6;
					++MyCaret.InsertionPoint;
				}
			} 
			CurrentSelectedArray = CurrentArray;
		}
		draw_document(LinePointers, MainDisplay, MainWindow, Context, MyCaret, Rectangles, nRectangles);
	}
}

