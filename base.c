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

struct StringArray *array_constructor()
{
	struct StringArray *MyArray = (struct StringArray *) malloc(sizeof(struct StringArray));
	MyArray->length = 0;
	MyArray->allocated = 100;
	MyArray->string = malloc(100);
	MyArray->string[0] = '\0';
	return MyArray;
}

void draw_document(struct StringArray* PointerArray[], Display* MainDisplay, Window MainWindow, GC Context, caret MyCaret)
{
	XClearWindow(MainDisplay, MainWindow);
	int CoordinateX = 10;
	int CoordinateY = 20;
	for(int i=0;PointerArray[i]!=NULL;++i){
		XDrawString(MainDisplay, MainWindow, Context, CoordinateX, CoordinateY, PointerArray[i]->string, PointerArray[i]->length);
		CoordinateY += 15;
	}
	XDrawLine(MainDisplay, MainWindow, Context, MyCaret.topX, MyCaret.topY, MyCaret.bottomX, MyCaret.bottomY);
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
	WindowAttributes.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | ExposureMask;

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
	ptrdiff_t position = 0;
	caret MyCaret = {10, 10, 10, 20, LinePointers[0]->string};
	while(IsWindowOpen) {
		XEvent GeneralEvent = {};
		XNextEvent(MainDisplay, &GeneralEvent);
		if(GeneralEvent.type == KeyPress){
			XKeyEvent *Event = &GeneralEvent.xkey;
			if(AllocatedLines <= nLinePointers+1) {
				expand_array(LinePointers, &AllocatedLines);
			}
			/*Backspace*/
			if(Event->keycode == 22){
				position = false;
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
				position = false;
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
				position = false;
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
				position = false;
				if(MyCaret.topX != 10){
					MyCaret.topX -= 6;
					MyCaret.bottomX -= 6;
					--MyCaret.InsertionPoint;
				}
			}
			/*Right*/
			else if(Event->keycode == 114){
				position = false;
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
					if(TruePosition!=true){
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
					if(TruePosition!=true){
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
		}
		draw_document(LinePointers, MainDisplay, MainWindow, Context, MyCaret);
		/*XClearWindow(MainDisplay, MainWindow);
		XDrawString(MainDisplay, MainWindow, Context, CurrentX, CurrentY, MyArray->string, MyArray->length);
		XDrawString(MainDisplay, MainWindow, Context, CurrentX, 35, "T", 1);*/
	}
}

