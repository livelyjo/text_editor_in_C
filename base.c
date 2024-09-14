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

struct LinkNode {
	struct StringArray *data;
	struct StringArray *next;
	struct StringArray *prev;
};

struct LinkedList {
	struct LinkNode *head = NULL;
	struct LinkNode *tail = NULL;
};

struct LinkedList *linked_list_constructor(struct *StringArray Array)
{
	struct LinkedList *list = (struct LinkedList *) malloc(sizeof(struct LinkedList));
	struct LinkNode *node = (struct LinkNode *) malloc(sizeof(struct LinkNode));
	node->data = Array;
	node->next = NULL;
	node->prev = NULL;
	list->head = node;
	list->tail = node;
	return list;
}

struct StringArray *array_constructor()
{
	struct StringArray *MyArray = (struct StringArray *) malloc(sizeof(struct StringArray));
	MyArray->length = 0;
	MyArray->allocated = 100;
	MyArray->string = malloc(100);
	return MyArray;
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

	struct StringArray *MyArray = array_constructor();
	struct LinkedList *MyLinkedList = linked_list_constructor(MyArray);
	struct LinkNode *CurrentNode = MyLinkedList->head;
	/*Shifting to a Linked List structure. Need to figure out how that changes the way I do the data manipulations below*/
	int IsWindowOpen = 1;
	char buffer_return[1000];
	int bytes_buffer;
	KeySym keysym_return;
	Status status_return;
	int Test;
	int nitems = 1;
	int CurrentString = 1;
	int StartingX = 10;
	int StartingY = 20;
	int CurrentX = StartingX;
	int CurrentY = StartingY;
	while(IsWindowOpen) {
		XEvent GeneralEvent = {};
		XNextEvent(MainDisplay, &GeneralEvent);
		if(GeneralEvent.type == KeyPress){
			XKeyEvent *Event = &GeneralEvent.xkey;
			if(Event->keycode == 22){
				--MyArray->length;
			}
			else if(((MyArray->allocated)-(MyArray->length))<=4) expand_array(MyArray);
			else if(Event->keycode == 23){
				for(int i=4;i>0;--i){
				MyArray->string[MyArray->length] = ' ';
				MyArray->length++;
				}
			}
			else if(Event->keycode == 36){
				MyArray->string[MyArray->length- = '\n';
				MyArray->length++;

			}
			else {
				bytes_buffer = sizeof(buffer_return);
				Test = XmbLookupString(StringContext, Event, buffer_return, bytes_buffer, &keysym_return, &status_return);
				MyArray->string[MyArray->length] = *buffer_return;
				++MyArray->length;
			} 
		}
		XClearWindow(MainDisplay, MainWindow);
		XDrawString(MainDisplay, MainWindow, Context, CurrentX, CurrentY, MyArray->string, MyArray->length);
		XDrawString(MainDisplay, MainWindow, Context, CurrentX, 35, "T", 1);
	}
}

