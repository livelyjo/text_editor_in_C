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

struct StringArray *array_constructor()
{
	struct StringArray *MyArray = (struct StringArray *) malloc(sizeof(struct StringArray));
	MyArray->length = 0;
	MyArray->allocated = 100;
	MyArray->string = malloc(100);
	return MyArray;
}

void draw_document(struct StringArray* PointerArray[], Display* MainDisplay, Window MainWindow, GC Context)
{
	XClearWindow(MainDisplay, MainWindow);
	int CoordinateX = 10;
	int CoordinateY = 20;
	for(int i=0;PointerArray[i]!=NULL;++i){
		XDrawString(MainDisplay, MainWindow, Context, CoordinateX, CoordinateY, PointerArray[i]->string, PointerArray[i]->length);
		CoordinateY += 15;
	}
}

void expand_array(struct StringArray *MyArray)
{
	MyArray->allocated *= 2;
	MyArray->string = realloc(MyArray->string, MyArray->allocated);
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
	struct StringArray* LinePointers[10] = {NULL};
	LinePointers[0] = array_constructor();
	int CurrentArray = 0;
	int IsWindowOpen = 1;
	char buffer_return[1000];
	int bytes_buffer;
	KeySym keysym_return;
	Status status_return;
	int Test;
	while(IsWindowOpen) {
		XEvent GeneralEvent = {};
		XNextEvent(MainDisplay, &GeneralEvent);
		if(GeneralEvent.type == KeyPress){
			XKeyEvent *Event = &GeneralEvent.xkey;
			if(Event->keycode == 22){
				if((LinePointers[CurrentArray]->length)==0 && CurrentArray!=0){
					struct StringArray* Next = LinePointers[CurrentArray+1];
					for(int i=CurrentArray;LinePointers[i]!=NULL;++i){
						LinePointers[i] = Next;
						Next = LinePointers[i+1];
					}
					--CurrentArray;
				}
				else if((LinePointers[CurrentArray]->length)>0)
					--LinePointers[CurrentArray]->length;
			}
			/*else if(((MyArray->allocated)-(MyArray->length))<=4) expand_array(MyArray);*/
			else if(Event->keycode == 23){
				for(int i=4;i>0;--i){
					LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length] = ' ';
					LinePointers[CurrentArray]->length++;
				}
			}
			else if(Event->keycode == 36){
				if(LinePointers[CurrentArray+1]!=NULL){
					struct StringArray* Current = LinePointers[CurrentArray];
					LinePointers[CurrentArray] = array_constructor();
					struct StringArray* Previous;
					int i;
					for(i=CurrentArray+1;LinePointers[i]!=NULL;++i){
						Previous = Current;
						Current = LinePointers[i];
						LinePointers[i] = Previous;
					}
					LinePointers[i] = Current;
					++CurrentArray;
				}
				else {
					++CurrentArray;
					LinePointers[CurrentArray] = array_constructor();
				}

			}
			else {
				bytes_buffer = sizeof(buffer_return);
				Test = XmbLookupString(StringContext, Event, buffer_return, bytes_buffer, &keysym_return, &status_return);
				LinePointers[CurrentArray]->string[LinePointers[CurrentArray]->length] = *buffer_return;
				++LinePointers[CurrentArray]->length;
			} 
		}
		draw_document(LinePointers, MainDisplay, MainWindow, Context);
		/*XClearWindow(MainDisplay, MainWindow);
		XDrawString(MainDisplay, MainWindow, Context, CurrentX, CurrentY, MyArray->string, MyArray->length);
		XDrawString(MainDisplay, MainWindow, Context, CurrentX, 35, "T", 1);*/
	}
}

