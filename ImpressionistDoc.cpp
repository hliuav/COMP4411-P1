// 
// impressionistDoc.cpp
//
// It basically maintain the bitmap for answering the color query from the brush.
// It also acts as the bridge between brushes and UI (including views)
//

#include <FL/fl_ask.H>
#include <math.h>

#include "impressionistDoc.h"
#include "impressionistUI.h"

#include "ImpBrush.h"

// Include individual brush headers here.
#include "PointBrush.h"
#include "LineBrush.h"
#include "CircleBrush.h"
#include "ScatteredPointBrush.h"
#include "ScatteredLineBrush.h"
#include "ScatteredCircleBrush.h"
#include "TriangleBrush.h"
#include "HeartBrush.h"
#include "BlurBrush.h"
#include "SharpenBrush.h"
#include "AlphaMappedBrush.h"
#include "WarpBrush.h"
#include "GrayBrush.h"


#define DESTROY(p)	{  if ((p)!=NULL) {delete [] p; p=NULL; } }


ImpressionistDoc::ImpressionistDoc()
{
	// Set NULL image name as init. 
	m_imageName[0]	='\0';	

	m_nWidth		= -1;
	m_ucBitmap		= NULL;
	m_ucBitmapR 	= NULL;
	m_ucBitmapG 	= NULL;
	m_ucBitmapB 	= NULL;
	m_ucAnotherBitmap = NULL;
	m_ucEdgeBitmap = NULL;
	m_ucUserEdgeBitmap = NULL;
	m_ucDissolveBitmap = NULL;
	m_ucPainting	= NULL;
	m_ucDisplayBitmap = NULL;
	m_ucPrePainting = NULL;
	m_ucMuralBitmap = NULL;
	m_ucAlphaBrushBitmap = NULL;

	m_nGradientxy = NULL;
	m_nGradientValue = NULL;
	m_nAnotherGradientxy = NULL;
	m_nAnotherGradientValue = NULL;
	firstTime = true;
	lastDimmedValue = 0;
	// create one instance of each brush
	ImpBrush::c_nBrushCount	= NUM_BRUSH_TYPE;
	ImpBrush::c_pBrushes	= new ImpBrush* [ImpBrush::c_nBrushCount];

	ImpBrush::c_pBrushes[BRUSH_POINTS]	= new PointBrush( this, "Points" );

	// Note: You should implement these 5 brushes.  They are set the same (PointBrush) for now
	ImpBrush::c_pBrushes[BRUSH_LINES]				
		= new LineBrush( this, "Lines" );
	ImpBrush::c_pBrushes[BRUSH_CIRCLES]				
		= new CircleBrush( this, "Circles" );
	ImpBrush::c_pBrushes[BRUSH_SCATTERED_POINTS]	
		= new ScatteredPointBrush( this, "Scattered Points" );
	ImpBrush::c_pBrushes[BRUSH_SCATTERED_LINES]		
		= new ScatteredLineBrush( this, "Scattered Lines" );
	ImpBrush::c_pBrushes[BRUSH_SCATTERED_CIRCLES]	
		= new ScatteredCircleBrush( this, "Scattered Circles" );
	ImpBrush::c_pBrushes[BRUSH_TRIANGLES]	
		= new TriangleBrush( this, "Tiragnles" );
	ImpBrush::c_pBrushes[BRUSH_HEARTS]	
		= new HeartBrush( this, "Hearts" );
	ImpBrush::c_pBrushes[BRUSH_BLUR]				
		= new BlurBrush( this, "Blur" );
	ImpBrush::c_pBrushes[BRUSH_SHARPEN]				
		= new SharpenBrush( this, "Sharpen" );
	ImpBrush::c_pBrushes[BRUSH_ALPHA_MAPPED]				
		= new AlphaMappedBrush( this, "Alpha Mapped" );
	ImpBrush::c_pBrushes[BRUSH_WARP]				
		= new WarpBrush( this, "Warp" );
	ImpBrush::c_pBrushes[BRUSH_GRAY]				
		= new GrayBrush( this, "Gray" );		

	// make one of the brushes current
	m_pCurrentBrush	= ImpBrush::c_pBrushes[0];
	m_nType = 0;
	m_nDirection = 0;
	m_nPaintlyStrokeType = 1;

	// init filter
	int SobelX[9] = { -1, 0, 1,
	-2, 0, 2,
	-1, 0, 1 }; //NOTE: Y from bottom to top

	int SobelY[9] = { -1, -2, -1,
	0, 0, 0,
	1, 2, 1 };

	double Gaussian[9] = { 0.0625, 0.125, 0.0625,
	0.125, 0.250, 0.125,
	0.0625, 0.125, 0.0625 };
	
	int Sharpen[9] = {
		0, -1, 0,
		-1, 5, -1,
		0, -1, 0
	};
	f_SobelX = new Filter<int>(SobelX,3,3);
	f_SobelY = new Filter<int>(SobelY,3,3);
	f_Gaussian = new Filter<double>(Gaussian,3,3);
	f_Sharpen = new Filter<int>(Sharpen,3,3);
	f_blur = NULL;
	

}


//---------------------------------------------------------
// Set the current UI 
//---------------------------------------------------------
void ImpressionistDoc::setUI(ImpressionistUI* ui) 
{
	m_pUI	= ui;
}

//---------------------------------------------------------
// Returns the active picture/painting name
//---------------------------------------------------------
char* ImpressionistDoc::getImageName() 
{
	return m_imageName;
}

//---------------------------------------------------------
// Called by the UI when the user changes the brush type.
// type: one of the defined brush types.
//---------------------------------------------------------
void ImpressionistDoc::setBrushType(int type)
{
	m_pCurrentBrush	= ImpBrush::c_pBrushes[type];
	m_nType = type;
	if (type == BRUSH_LINES || type == BRUSH_SCATTERED_LINES)
	{
		m_pUI->m_BrushLineWidthSlider->activate();
		m_pUI->m_BrushLineAngleSlider->activate();
		m_pUI->m_BrushDirectionChoice->activate();
		m_pUI->m_EdgeClippingButton->activate();
	}
	else {
		m_pUI->m_BrushLineWidthSlider->deactivate();
		m_pUI->m_BrushLineAngleSlider->deactivate();
		m_pUI->m_BrushDirectionChoice->deactivate();
		m_pUI->m_EdgeClippingButton->deactivate();
		m_pUI->m_AnotherGradientButton->deactivate();
	}
	if (type == BRUSH_BLUR || type == BRUSH_SHARPEN)
	{
		m_pUI->m_BrushSizeSlider->deactivate();
	}
	else m_pUI->m_BrushSizeSlider->activate();

	if (type == BRUSH_WARP)
	{
		setPaintingDone();
	}
}

void ImpressionistDoc::setPaintlyStrokeType(int type)
{
	m_nPaintlyStrokeType = type;
}


//---------------------------------------------------------
// Called by the UI when the user changes the brush direction.
// type: one of the defined brush types.
//---------------------------------------------------------
void ImpressionistDoc::setBrushDirection(int type)
{	
	m_nDirection = type;
	if (type == DIRECTION_SLIDER)
	{
		m_pUI->m_BrushLineAngleSlider->activate();
		
	}
	else if (type == DIRECTION_GRADIENT)
	{
		m_pUI->m_AnotherGradientButton->activate();
	}
	else 
	{	
		m_pUI->m_BrushLineAngleSlider->deactivate();
		m_pUI->m_AnotherGradientButton->deactivate();
	}

}
void ImpressionistDoc:: setSize(int size){
	m_pUI->setSize(size);
}

void ImpressionistDoc::setLineAngle(int angle){
	m_pUI->setLineAngle(angle);
}

void ImpressionistDoc::setStartPoint(Point start){
	startPoint=start;
}

void ImpressionistDoc::setEndPoint(Point end){
	endPoint=end;
}

int ImpressionistDoc::rightMouseAngle(){
	int result = atan2(endPoint.y - startPoint.y, endPoint.x - startPoint.x) * 180 / 3.14159265;
	if (result < 0)
	{
		result+=360;
	}
	return result;
}
int ImpressionistDoc::rightMouseSize(){
	int x1=startPoint.x;
	int x2=endPoint.x;
	int y1=startPoint.y;
	int y2=endPoint.y;
	int del_x=x2-x1;
	int del_y=y2-y1;
	int result =  sqrt(del_x*del_x + del_y*del_y);
	return result;
}

//---------------------------------------------------------
// Returns the size of the brush.
//---------------------------------------------------------
int ImpressionistDoc::getSize()
{
	return m_pUI->getSize();
}

int ImpressionistDoc::getBrushType(){
	return m_nType;
}

int ImpressionistDoc::getDirectionType(){
	return m_nDirection;
}

//---------------------------------------------------------
// Returns the size of the brush.
//---------------------------------------------------------
int ImpressionistDoc::getLineWidth()
{
	return m_pUI->getLineWidth();
}

//---------------------------------------------------------
// Returns the size of the brush.
//---------------------------------------------------------
int ImpressionistDoc::getLineAngle()
{
	return m_pUI->getLineAngle();
}

//---------------------------------------------------------
// Returns the opacity of the brush.
//---------------------------------------------------------
double ImpressionistDoc::getOpacity()
{
	return m_pUI->getOpacity();
}

//---------------------------------------------------------
// Load the specified image
// This is called by the UI when the load image button is 
// pressed.
//---------------------------------------------------------
int ImpressionistDoc::loadImage(char *iname) 
{
	// try to open the image to read
	unsigned char*	data;
	int				width, 
					height;

	if ( (data=readBMP(iname, width, height))==NULL ) 
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}

	// reflect the fact of loading the new image
	m_nWidth		= width;
	m_nPaintWidth	= width;
	m_nHeight		= height;
	m_nPaintHeight	= height;

	// release old storage
	if ( m_ucBitmap ) delete [] m_ucBitmap;
	if ( m_ucPainting ) delete [] m_ucPainting;
	if ( m_ucEdgeBitmap) delete[] m_ucEdgeBitmap;
	if ( m_nGradientxy)	delete[] m_nGradientxy;
	if ( m_nGradientValue) delete[] m_nGradientValue;
	if ( m_ucDisplayBitmap) delete[] m_ucDisplayBitmap;
	
	m_ucBitmap		= data;
	generateRGB();
	// allocate space for draw view
	m_ucPainting	= new unsigned char [width*height*3];
	memset(m_ucPainting, 0, width*height*3);
	m_ucDisplayBitmap	= new unsigned char [width*height*3];
	memset(m_ucDisplayBitmap, 0, width*height*3);

	m_pUI->m_mainWindow->resize(m_pUI->m_mainWindow->x(), 
								m_pUI->m_mainWindow->y(), 
								width*2, 
								height+25);

	// Calculate Gradient
	m_nGradientxy = new int [2 * width * height];
	m_nGradientValue = new int [width * height];
	calculateGradient(m_ucBitmap, m_nGradientxy, m_nGradientValue);

	// allocate space for edge view
	m_ucEdgeBitmap = new GLubyte[3*width*height];
	calculateEdgeMap(m_pUI->getThreshold());

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}

void ImpressionistDoc:: generateRGB(){
	if (m_ucBitmapR) delete[] m_ucBitmapR;
	if (m_ucBitmapG) delete[] m_ucBitmapG;
	if (m_ucBitmapB) delete[] m_ucBitmapB;
	m_ucBitmapR = new GLubyte [m_nWidth * m_nHeight];
	m_ucBitmapG = new GLubyte [m_nWidth * m_nHeight];
	m_ucBitmapB = new GLubyte [m_nWidth * m_nHeight];
	for (int i = 0; i < m_nWidth; ++i)
	{
		for (int j = 0; j < m_nHeight; ++j)
		{
			m_ucBitmapR[j * m_nWidth +i] = m_ucBitmap[3 * (j * m_nWidth + i)];
			m_ucBitmapG[j * m_nWidth +i] = m_ucBitmap[3 * (j * m_nWidth + i)+1];
			m_ucBitmapB[j * m_nWidth +i] = m_ucBitmap[3 * (j * m_nWidth + i)+2];
		}
	}
}

//---------------------------------------------------------
// Change threshold
// This is called by the UI when the load image button is 
// pressed.
//---------------------------------------------------------
int ImpressionistDoc::changeThreshold() 
{
	int width = m_nWidth;
	int height = m_nHeight;
	// release old storage
	if ( m_ucEdgeBitmap) delete[] m_ucEdgeBitmap;

	// allocate space for edge view
	m_ucEdgeBitmap = new GLubyte[3*width*height];
	calculateEdgeMap(m_pUI->getThreshold());

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}

//---------------------------------------------------------
// Dissolve the specified image
// This is called by the UI when the load image button is 
// pressed.
//---------------------------------------------------------
int ImpressionistDoc::dissolveImage(char *iname) 
{
	// try to open the image to read
	unsigned char*	data;
	int				width, 
					height;

	if ( (data=readBMP(iname, width, height))==NULL ) 
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}
	if (width != m_nWidth || height != m_nHeight)
	{
		fl_alert("Different size");
		return 0;
	}

	// reflect the fact of loading the new image
	m_nWidth		= width;
	m_nPaintWidth	= width;
	m_nHeight		= height;
	m_nPaintHeight	= height;

	m_ucDissolveBitmap		= data;

	double alpha = 0.5;
	for (int i = 0; i < 3 * m_nWidth * m_nHeight; ++i)
	{
		m_ucBitmap[i] = m_ucBitmap[i] * (1 - alpha * 1) + m_ucDissolveBitmap[i] * alpha * 1; // alpha mask set to all 1
	}
	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}

// Load edge Image
int ImpressionistDoc::loadEdgeImage(char *iname) 
{
	// try to open the image to read
	unsigned char*	data;
	int				width, 
					height;

	if ( (data=readBMP(iname, width, height))==NULL ) 
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}

	if (width != m_nWidth || height != m_nHeight)
	{
		fl_alert("Different size");
		return 0;
	}

	// reflect the fact of loading the new image
	m_nWidth		= width;
	m_nPaintWidth	= width;
	m_nHeight		= height;
	m_nPaintHeight	= height;

	// release old storage
	if ( m_ucUserEdgeBitmap) delete[] m_ucUserEdgeBitmap;

	m_ucUserEdgeBitmap		= data;

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}

//---------------------------------------------------------
// Load the another image
// This is called by the UI when the load image button is 
// pressed.
//---------------------------------------------------------

int ImpressionistDoc::loadAnotherImage(char *iname) 
{
	// try to open the image to read
	unsigned char*	data;
	int				width, 
					height;

	if ( (data=readBMP(iname, width, height))==NULL ) 
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}

	if (width != m_nWidth || height != m_nHeight)
	{
		fl_alert("Different size");
		return 0;
	}

	if ( m_nAnotherGradientxy) delete[] m_nAnotherGradientxy;
	if ( m_nAnotherGradientValue) delete[] m_nAnotherGradientValue;

	// reflect the fact of loading the new image
	m_nWidth		= width;
	m_nPaintWidth	= width;
	m_nHeight		= height;
	m_nPaintHeight	= height;

	// release old storage

	m_ucAnotherBitmap = data;

	// Calculate Gradient
	m_nAnotherGradientxy = new int [2 * width * height];
	m_nAnotherGradientValue = new int [width * height];
	calculateGradient(m_ucAnotherBitmap, m_nAnotherGradientxy, m_nAnotherGradientValue);

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}

//---------------------------------------------------------
// Load the mural image
// This is called by the UI when the load image button is 
// pressed.
//---------------------------------------------------------
int ImpressionistDoc::loadMuralImage(char *iname) 
{
	// try to open the image to read
	unsigned char*	data;
	int				width, 
					height;

	if ( (data=readBMP(iname, width, height))==NULL ) 
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}

	if (width != m_nWidth || height != m_nHeight)
	{
		fl_alert("Different size");
		return 0;
	}
	// reflect the fact of loading the new image
	m_nWidth		= width;
	m_nPaintWidth	= width;
	m_nHeight		= height;
	m_nPaintHeight	= height;

	// release old storage
	if ( m_ucBitmap ) delete [] m_ucBitmap;
	if ( m_ucEdgeBitmap) delete[] m_ucEdgeBitmap;
	if ( m_nGradientxy)	delete[] m_nGradientxy;
	if ( m_nGradientValue) delete[] m_nGradientValue;

	m_ucBitmap		= data;
	generateRGB();
	// Calculate Gradient
	m_nGradientxy = new int [2 * width * height];
	m_nGradientValue = new int [width * height];
	calculateGradient(m_ucBitmap, m_nGradientxy, m_nGradientValue);

	// allocate space for edge view
	m_ucEdgeBitmap = new GLubyte[3*width*height];
	calculateEdgeMap(m_pUI->getThreshold());

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();

	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();


	return 1;
}
//---------------------------------------------------------
// Load the alpha image
// This is called by the UI when the load alpha brush button is 
// pressed.
//---------------------------------------------------------
int ImpressionistDoc::loadAlphaMap(char* iname)
{
	// try to open the image to read
	unsigned char*	data;
	int				width,
		height;

	if ((data = readBWBMP(iname, width, height)) == NULL)
	{
		fl_alert("Can't load bitmap file");
		return 0;
	}

	if (width != m_nWidth || height != m_nHeight)
	{
		fl_alert("Different size");
		return 0;
	}

	m_nAlphaBrushWidth = width;
	m_nAlphaBrushHeight = height;

	//load the bit map
	if (m_ucAlphaBrushBitmap) delete[] m_ucAlphaBrushBitmap;
	m_ucAlphaBrushBitmap = data;

	return 1;
}

//----------------------------------------------------------------
// Swap the views
// This is called by the UI when the swap views menu button is 
// pressed.
//----------------------------------------------------------------

void ImpressionistDoc::swapViews(){
	// release old storage
	// if ( m_ucBitmap ) delete [] m_ucBitmap;
	// if ( m_ucPainting ) delete [] m_ucPainting;
	int width, height;
	width = m_nWidth;
	height = m_nHeight;
	unsigned char* temp = m_ucBitmap;
	m_ucBitmap		= m_ucPainting;

	// // allocate space for draw view
	// m_ucPainting	= new unsigned char [width*height*3];
	// memset(m_ucPainting, 0, width*height*3);

	// m_pUI->m_mainWindow->resize(m_pUI->m_mainWindow->x(), 
	// 							m_pUI->m_mainWindow->y(), 
	// 							width*2, 
	// 							height+25);

	m_ucPainting = temp;

	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();
	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();
}

void ImpressionistDoc::saveTemp()
{
	int width, height;
	width = m_nWidth;
	height = m_nHeight;
	if ( m_ucPrePainting ) delete [] m_ucPrePainting;
	m_ucPrePainting	= new unsigned char [width*height*3];
	memcpy ( m_ucPrePainting, m_ucPainting, width*height*3 );
}

void ImpressionistDoc::saveDim()
{
	int width, height;
	width = m_nWidth;
	height = m_nHeight;
	if ( m_ucDisplayBitmap ) delete [] m_ucDisplayBitmap;
	m_ucDisplayBitmap	= new unsigned char [width*height*3];
	memcpy ( m_ucDisplayBitmap, m_ucPainting, width*height*3 );
}

void ImpressionistDoc::undo(){
	int width, height;
	width = m_nWidth;
	height = m_nHeight;
	if ( m_ucPainting ) delete [] m_ucPainting;
	m_ucPainting	= new unsigned char [width*height*3];
	memcpy ( m_ucPainting, m_ucPrePainting, width*height*3 );
	// display it on origView
	m_pUI->m_origView->resizeWindow(width, height);	
	m_pUI->m_origView->refresh();
	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(width, height);	
	m_pUI->m_paintView->refresh();
}

//----------------------------------------------------------------
// Save the specified image
// This is called by the UI when the save image menu button is 
// pressed.
//----------------------------------------------------------------
int ImpressionistDoc::saveImage(char *iname) 
{

	writeBMP(iname, m_nPaintWidth, m_nPaintHeight, m_ucPainting);

	return 1;
}

//----------------------------------------------------------------
// Clear the drawing canvas
// This is called by the UI when the clear canvas menu item is 
// chosen
//-----------------------------------------------------------------
int ImpressionistDoc::clearCanvas() 
{

	// Release old storage
	if ( m_ucPainting ) 
	{
		delete [] m_ucPainting;

		// allocate space for draw view
		m_ucPainting	= new unsigned char [m_nPaintWidth*m_nPaintHeight*3];
		memset(m_ucPainting, 0, m_nPaintWidth*m_nPaintHeight*3);

		// refresh paint view as well	
		m_pUI->m_paintView->refresh();
	}
	
	return 0;
}

//------------------------------------------------------------------
// Get the color of the pixel in the original image at coord x and y
//------------------------------------------------------------------
GLubyte* ImpressionistDoc::GetOriginalPixel( int x, int y )
{
	if ( x < 0 ) 
		x = 0;
	else if ( x >= m_nWidth ) 
		x = m_nWidth-1;

	if ( y < 0 ) 
		y = 0;
	else if ( y >= m_nHeight ) 
		y = m_nHeight-1;

	return (GLubyte*)(m_ucBitmap + 3 * (y*m_nWidth + x));
}

//----------------------------------------------------------------
// Get the color of the pixel in the original image at point p
//----------------------------------------------------------------
GLubyte* ImpressionistDoc::GetOriginalPixel( const Point p )
{
	return GetOriginalPixel( p.x, p.y );
}

//------------------------------------------------------------------
// Get the color of the pixel in the painting image at coord x and y
//------------------------------------------------------------------
GLubyte* ImpressionistDoc::GetPaintingPixel( int x, int y )
{
	if ( x < 0 ) 
		x = 0;
	else if ( x >= m_nWidth ) 
		x = m_nWidth-1;

	if ( y < 0 ) 
		y = 0;
	else if ( y >= m_nHeight ) 
		y = m_nHeight-1;

	return (GLubyte*)(m_ucBitmap + 3 * (y*m_nWidth + x));
}

//----------------------------------------------------------------
// Get the color of the pixel in the painting image at point p
//----------------------------------------------------------------
GLubyte* ImpressionistDoc::GetPaintingPixel( const Point p )
{
	return GetPaintingPixel( p.x, p.y );
}

//----------------------------------------------------------------
// Get the color of the pixel in the painting image at point p double
//----------------------------------------------------------------
GLubyte* ImpressionistDoc::GetPaintingPixel(double x, double y)
{
	int ceilx = ceil(x);
	int ceily = ceil(y);
	int floorx = floor(x);
	int floory = floor(y);
	if (ceilx == floorx && ceily == floory) return GetPaintingPixel((int)x, (int)y);
	if (ceilx == floorx || ceily == floory)
	{
		GLubyte* pixel1 = GetPaintingPixel(ceilx, ceily);
		GLubyte* pixel2 = GetPaintingPixel(floorx, floory);
		//take average of the two pixels
		GLubyte* pixel = new GLubyte[3];
		for (int i = 0; i < 3; ++i)
		{
			pixel[i] = (pixel1[i] + pixel2[i]) / 2;
		}
		return pixel;
	}
	GLubyte* pixel1 = GetPaintingPixel(ceilx, ceily);
	GLubyte* pixel2 = GetPaintingPixel(ceilx, floory);
	GLubyte* pixel3 = GetPaintingPixel(floorx, ceily);
	GLubyte* pixel4 = GetPaintingPixel(floorx, floory);
	//take average of the two pixels
	GLubyte* pixel = new GLubyte[3];
	for (int i = 0; i < 3; ++i)
	{
		pixel[i] = (pixel1[i] + pixel2[i] + pixel3[i] + pixel4[i]) / 4;
	}
	return pixel;
}

GLubyte* ImpressionistDoc::GetPaintingPixel(const Pointd p)
{
	return GetPaintingPixel( p.x, p.y );
}

//------------------------------------------------------------------
// SET the color of the pixel in the paint image at coord x and y
//------------------------------------------------------------------
void ImpressionistDoc::SetPaintingPixel(int x, int y, const GLubyte* color)
{
	//directly ignore outbound situations
	if (x < m_nWidth && y < m_nHeight && x > 0 && y > 0)
		memcpy(m_ucPainting + 3 * (y * m_nWidth + x), color, 3);
}
void ImpressionistDoc::SetPaintingPixel(const Point p, const GLubyte* color)
{
	SetPaintingPixel(p.x, p.y, color);
}

GLubyte* ImpressionistDoc::GetAnotherPixel( int x, int y )
{
	if ( x < 0 ) 
		x = 0;
	else if ( x >= m_nWidth ) 
		x = m_nWidth-1;

	if ( y < 0 ) 
		y = 0;
	else if ( y >= m_nHeight ) 
		y = m_nHeight-1;

	return (GLubyte*)(m_ucAnotherBitmap + 3 * (y*m_nWidth + x));
}

//----------------------------------------------------------------
// Get the color of the pixel in the original image at point p
//----------------------------------------------------------------
GLubyte* ImpressionistDoc::GetAnotherPixel( const Point p )
{
	return GetAnotherPixel( p.x, p.y );
}

void ImpressionistDoc::calculateGradient(unsigned char* source, int* gradientxy, int* value ){
	int width = m_nPaintWidth, height = m_nPaintHeight;

	//calculate the intensity
	GLubyte * grayImage = new GLubyte[width * height];
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < height; ++j)
		{
			grayImage[j * width + i] = grayPixel(&source[3 * (j * width + i)]);
		}
	}

	//Apply gaussian filter to blur
	GLubyte* blurredImage = new GLubyte[width*height];
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < height; ++j)
		{
			blurredImage[j * width + i] = f_Gaussian->applyToPixel(grayImage, width, height, i, j);
			// printf("%d\n", blurredImage[j * width + i]);
		}
	}
	//Calculate the gradient
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < height; ++j)
		{
			int pos = i + j * width;
			gradientxy[2 * pos] = f_SobelX->applyToPixel(blurredImage, width, height, i, j);
			gradientxy[2 * pos + 1] = f_SobelY->applyToPixel(blurredImage, width, height, i, j);
			value[pos] = sqrt(pow(gradientxy[2 * pos], 2) + pow(gradientxy[2 * pos + 1], 2));
		}
	}
}

// Calculate edge with given threshold, and store it.
GLubyte* ImpressionistDoc::calculateEdgeMap(int threshold)
{
	if (!m_ucEdgeBitmap) return NULL;

	int pos = 0;
	for (int i = 0; i < m_nPaintWidth; ++i)
		for (int j = 0; j < m_nPaintHeight; ++j)
		{
			pos = (j * m_nPaintWidth + i);
			m_ucEdgeBitmap[3*pos] = m_nGradientValue[pos] > threshold ? 255 : 0;
			m_ucEdgeBitmap[3*pos+1] = m_nGradientValue[pos] > threshold ? 255 : 0;
			m_ucEdgeBitmap[3*pos+2] = m_nGradientValue[pos] > threshold ? 255 : 0;
		}
	//return the calculated Edge
	return m_ucEdgeBitmap;
}

GLubyte ImpressionistDoc:: grayPixel(double r, double g, double b)
{
	double result = 0.299 * r + 0.587 * g + 0.144 * b;
	return (result > 255) ? 255 : (GLubyte) result; //cap the value to prevent overflow
}

GLubyte ImpressionistDoc:: grayPixel( GLubyte*  pixel)
{
	return grayPixel(pixel[0], pixel[1], pixel[2]);
}

void ImpressionistDoc:: applyUserFilter(){
	int w = m_pUI->getKernelWidth();
	int h = m_pUI->getKernelHeight();
	double* kernel = new double[w*h];
	double sum = 0;
	for (int i = 0; i < w; ++i)
	{
		for (int j = 0; j < h; ++j)
		{
			int index = j * w +i;
			kernel[index] = atof(m_pUI->m_FilterInputs[index]->value());
			sum += kernel[index];
		}
	}
	if (m_pUI->getNormalize())
	{
		for (int i = 0; i < w * h; ++i)
		{
			kernel[i] /= sum;
		}
	}
	Filter<double>* userFilter = new Filter<double>(kernel, w, h);
	delete [] kernel;
	for (int i = 0; i < m_nWidth; ++i)
	{
		for (int j = 0; j < m_nHeight; ++j)
		{
			int index = 3 * (j * m_nWidth +i);
			m_ucPainting[index]=userFilter->applyToPixel(m_ucBitmapR,m_nWidth,m_nHeight,i,j);
			m_ucPainting[index+1]=userFilter->applyToPixel(m_ucBitmapG,m_nWidth,m_nHeight,i,j);
			m_ucPainting[index+2]=userFilter->applyToPixel(m_ucBitmapB,m_nWidth,m_nHeight,i,j);
		}
	}
	// display it on origView
	m_pUI->m_origView->resizeWindow(m_nWidth, m_nHeight);	
	m_pUI->m_origView->refresh();
	// refresh paint view as well
	m_pUI->m_paintView->resizeWindow(m_nWidth, m_nHeight);	
	m_pUI->m_paintView->refresh();
}

void ImpressionistDoc:: changeDimmedValue(){
	double dimmedValue = m_pUI->getDimmedValue();
	for (int i = 0; i < 3* m_nWidth * m_nHeight; ++i)
	{	
		if (m_pUI->getDimmedChoice())
		{	
			int temp = m_ucPainting[i];
			m_ucPainting[i] =  m_ucBitmap[i] * dimmedValue;
		}
		else
		{
			GLubyte temp = m_ucBitmap[i] * dimmedValue;
			if (m_ucPainting[i] == temp )
			{
				m_ucPainting[i] = 0;
			}
		}
		// m_ucDisplayBitmap[i] = m_ucBitmap[i] * dimmedValue + m_ucPainting[i] * (1 - dimmedValue);
		// printf("display: %d\n", m_ucDisplayBitmap[i]);
		// m_ucPainting[i] = dimmedValue * m_ucBitmap[i] + (1 - dimmedValue ) * m_ucPainting[i];
	}
	printf("%d\n",m_ucPainting[0] );
	for (int i = 0; i < 3 * m_nWidth * m_nHeight; ++i)
	{
		m_ucDisplayBitmap[i] = m_ucPainting[i];
	}
	// printf("display:%d\n", m_ucDisplayBitmap[0]);
	// printf("paint:%d\n", m_ucPainting[0]);
	// refresh paint view as well
	// Dimme abort
	// printf("dimmedValue: %f\n", dimmedValue);
	m_pUI->m_paintView->resizeWindow(m_nWidth, m_nHeight);	
	m_pUI->m_paintView->refresh();
	
	// m_ucPainting = temp;

	
}

void ImpressionistDoc::swap(GLubyte* &a, GLubyte* &b)
{
	GLubyte* temp = a;
	a = b;
	b =temp;
}

void ImpressionistDoc::setPaintingDone(){
	memcpy ( m_ucPainting, m_ucBitmap, m_nWidth*m_nHeight*3 );
	m_pUI->m_paintView->resizeWindow(m_nWidth, m_nHeight);	
	m_pUI->m_paintView->refresh();
}

void ImpressionistDoc:: generateBlur()
{
	double f_sigma = m_pUI->getPaintlyBlur();
	int kernel_size = 50 * f_sigma;
	int sigma = 7;
	double* kernel = new double[ kernel_size * kernel_size];
	double sum = 0;
	for (int i = 0; i < kernel_size; ++i)
	{
		for (int j = 0; j < kernel_size; ++j)
		{
			kernel[ i * kernel_size + j] = 1/(2 * 3.1415926 * sigma * sigma)*exp((-(i - kernel_size/2) * (i - kernel_size/2) - (j - kernel_size/2) * (j - kernel_size/2))/(sigma * sigma));
			sum += kernel[ i* kernel_size + j];
		}
	}
	// printf("sum:%f\n", sum);
	for (int i = 0; i < kernel_size * kernel_size; ++i)
	{
		kernel[i] /= sum;
		// printf("%f\n", kernel[i]);
	}
	f_blur = new Filter<double>(kernel, kernel_size, kernel_size);
}
