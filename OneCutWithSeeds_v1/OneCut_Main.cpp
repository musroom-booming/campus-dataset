


//




//@inproceedings{iccv2013onecut,




//	title	= {Grabcut in One Cut},




//	author	= {Tang, Meng and Gorelick, Lena and Veksler, Olga and Boykov, Yuri},




//	booktitle={International Conference on Computer Vision},




//	month	= {December},




//	year	= {2013}}




//




//THIS SOFTWARE USES maxflow/min-cut CODE THAT WAS IMPLEMENTED BY VLADIMIR KOLMOGOROV,




//THAT CAN BE DOWNLOADED FROM http://vision.csd.uwo.ca/code/.




//PLEASE USE THE FOLLOWING CITATION:




//




//@ARTICLE{Boykov01anexperimental,




//    author = {Yuri Boykov and Vladimir Kolmogorov},




//    title = {An Experimental Comparison of Min-Cut/Max-Flow Algorithms for Energy Minimization in Vision},




//    journal = {IEEE TRANSACTIONS ON PATTERN ANALYSIS AND MACHINE INTELLIGENCE},




//    year = {2001},




//    volume = {26},




//    pages = {359--374}}




//









//




//##################################################################




//




//  USAGE  INSTRUCTIONS




//




//	In the command line type:




//	




//	OneCut <imageFileName> [<beta> <numBinsPerChannel>]




//




//	Default values: beta= 0.1, numBinsPerChannel=64




//




//	Example: OneCut frida_small.jpg 0.1 64




//	or       OneCut frida_small.jpg 




//




//




//	Once the image is opened you can scribble with left and right




//	mouse buttons on the object and the background in the 




//	"Scribble Image" window. Once the scribbles are given you can 




//	segment the image.You can keep repeatedly adding scribbles and 




//	segmenting until the result is satisfactory.




//




//	Use the following Short Keys:




//		'q' - quit




//		's' - segment




//		'r' - reset (removes all strokes and clears all results)




//		'+' - increase brush stroke radius




//		'-' - decrease brush stroke radius




//		'right mouse button drug' - draw blue scribble




//		'left mouse button drug' - draw red scribble




//




//




#include <iostream> // for standard I/O




#include <string>   // for strings




#include <iomanip>  // for controlling float print precision 




#include <sstream>  // string to number conversion 









#include <opencv2/imgproc/imgproc.hpp>  // Gaussian Blur




#include <opencv2/core/core.hpp>        // Basic OpenCV structures (cv::Mat, Scalar)




#include <opencv2/highgui/highgui.hpp>  // OpenCV window I/O









#include "graph.h"




#include "ComputeTime.h"














using namespace std;




using namespace cv;














// images




Mat inputImg, showImg, binPerPixelImg, showEdgesImg, segMask, segShowImg;









// mask




Mat fgScribbleMask, bgScribbleMask;









// user clicked mouse buttons flags




bool rButtonDown = false;




bool lButtonDown = false;




int numUsedBins = 0;




float varianceSquared = 0;




int scribbleRadius = 5;//���ʰ뾶














// default arguments




float bha_slope = 0.1f;




int numBinsPerChannel = 64;














const float INT32_CONST = 1000;




const float HARD_CONSTRAINT_CONST = 1000;














#define NEIGHBORHOOD_8_TYPE 1;




#define NEIGHBORHOOD_25_TYPE 2;









const int NEIGHBORHOOD = NEIGHBORHOOD_8_TYPE;














//************************************




// F u n c t i o n     d e c l a r a t i o n s 









// init all images/vars




int  init(char * imgFileName);









// clear everything before closing




void destroyAll();









// mouse listener




static void onMouse(int event, int x, int y, int, void*);









// set bin index for each image pixel, store it in binPerPixelImg




void getBinPerPixel(Mat & binPerPixelImg, Mat & inputImg, int numBinsPerChannel, int & numUsedBins);









// compute the variance of image edges between neighbors




void getEdgeVariance(Mat & inputImg, Mat & showEdgesImg, float & varianceSquared);









typedef Graph<int, int, int> GraphType;




GraphType *myGraph;
























//***********************************




// M a i n 









/*




if( argc > 4 || argc < 2)




{




cout <<" Usage: seedsAndOverlap ImageToSegment [numBinsPerChannel bha_slope]" << endl;




return -1;




}




if (argc >= 3)




{




// get the second arg




String numBinsStr(argv[2]);









// convert to int




numBinsPerChannel = atoi(numBinsStr.c_str());




cout << "Using " << numBinsPerChannel <<  " bins per channel " << endl;




if (argc >=4)




{




//get third argument




String bhaSlopeStr(argv[3]);




bha_slope = (float)atof(bhaSlopeStr.c_str());




cout << "Using beta  = " << bha_slope << endl;




}




else




cout << "Using default beta = " << bha_slope << endl;




}




else




{




cout << "Using default " << numBinsPerChannel <<  " bins per channel " << endl;




cout << "Using default beta = " << bha_slope << endl;




}









*/









int main(int argc, char *argv[])




{














	String image_name, numBinsStr, bhaSlopeStr;




	cout << "input Parameters:" << endl;




	cout << "image name: ";




	cin >> image_name;




	cout << endl << "numBinsPerChannel: ";




	cin >> numBinsStr;




	cout << endl << "beta: ";




	cin >> bhaSlopeStr;









	// get img name parameter




	char * imgFileName = (char *)image_name.c_str();














	// convert to int 




	numBinsPerChannel = atoi(numBinsStr.c_str());




	cout << "Using " << numBinsPerChannel << " bins per channel " << endl;









	bha_slope = (float)atof(bhaSlopeStr.c_str());




	cout << "Using beta  = " << bha_slope << endl;









	//cout << "Using default beta = " << bha_slope << endl; 














	ComputeTime ct_init;//�����������ʱ�����




	ct_init.Begin();




	if (init(imgFileName) == -1)




	{




		cout << "Could not initialize" << endl;




		return -1;




	}














	cout << "��ʼ������ʱ�䣺  " << ct_init.End() << "ms" << endl;





























	// Wait for a keystroke in the window




	for (;;)




	{




		char key = waitKey(0);




		switch (key)




		{




		case 'q':




			cout << "goodbye" << endl;




			destroyAll();




			return 0;




		case '-':




			//��С����ֱ��




			if (scribbleRadius > 2)




				scribbleRadius--;




			cout << "current radius is " << scribbleRadius << endl;




			break;




		case '+':




			if (scribbleRadius < 100)




				scribbleRadius++;




			cout << "current radius is " << scribbleRadius << endl;




			break;




		case 's':




		{




					ComputeTime ct;//�����������ʱ�����




					ct.Begin();




					cout << "setting the hard constraints..." << endl;




					for (int i = 0; i<inputImg.rows; i++)




					{




						for (int j = 0; j<inputImg.cols; j++)




						{




							// this is the node id for the current pixel




							GraphType::node_id currNodeId = i * inputImg.cols + j;









							// add hard constraints based on scribbles




							if (fgScribbleMask.at<uchar>(i, j) == 255)




								myGraph->add_tweights(currNodeId, (int)ceil(INT32_CONST * HARD_CONSTRAINT_CONST + 0.5), 0);




							else if (bgScribbleMask.at<uchar>(i, j) == 255)




								myGraph->add_tweights(currNodeId, 0, (int)ceil(INT32_CONST * HARD_CONSTRAINT_CONST + 0.5));




						}




					}




					cout << "maxflow..." << endl;




					int flow = myGraph->maxflow();




					cout << "done maxflow..." << endl;









					// this is where we store the results




					segMask = 0;




					inputImg.copyTo(segShowImg);




					//inputImg.copyTo(showImg);









					// empty scribble masks are ready to record additional scribbles for additional hard constraints




					// to be used next time




					fgScribbleMask = 0;




					bgScribbleMask = 0;









					// copy the segmentation results on to the result images




					for (int i = 0; i<inputImg.rows * inputImg.cols; i++)




					{




						// if it is foreground - color blue




						if (myGraph->what_segment((GraphType::node_id)i) == GraphType::SOURCE)




						{




							segMask.at<uchar>(i / inputImg.cols, i%inputImg.cols) = 255;




							//(uchar)segShowImg.at<Vec3b>(i/inputImg.cols, i%inputImg.cols)[2] =  200;




						}




						// if it is background - color red




						else




						{




							segMask.at<uchar>(i / inputImg.cols, i%inputImg.cols) = 0;




							(uchar)segShowImg.at<Vec3b>(i / inputImg.cols, i%inputImg.cols)[0] = 0;




							(uchar)segShowImg.at<Vec3b>(i / inputImg.cols, i%inputImg.cols)[1] = 0;




							(uchar)segShowImg.at<Vec3b>(i / inputImg.cols, i%inputImg.cols)[2] = 0;




						}




					}









					imshow("Segmentation Mask", segMask);




					imshow("Segmentation Image", segShowImg);









					cout << "����ʱ�䣺  " << ct.End() << "ms" << endl;














					imwrite("seg_result.bmp", segShowImg);




					waitKey(0);




					break;









		}




		case 'r':




		{




					cout << "resetting" << endl;




					destroyAll();




					if (init(imgFileName) == -1)




					{




						cout << "could not initialize" << std::endl;




						return -1;




					}




					break;




		}




		}




	}














	return 0;




}









// mouse listener




static void onMouse(int event, int x, int y, int, void*)




{




	//cout << "On Mouse: (" << x << "," << y << ")" <<endl;














	if (event == CV_EVENT_LBUTTONDOWN)




	{




		lButtonDown = true;









	}




	else if (event == CV_EVENT_RBUTTONDOWN)




	{




		rButtonDown = true;









	}




	else if (event == CV_EVENT_LBUTTONUP)




	{




		lButtonDown = false;




	}




	else if (event == CV_EVENT_RBUTTONUP)




	{




		rButtonDown = false;




	}




	else if (event == CV_EVENT_MOUSEMOVE)




	{




		if (rButtonDown)




		{




			// scribble the background









			circle(bgScribbleMask, Point(x, y), scribbleRadius, 255, -1);




			circle(showImg, Point(x, y), scribbleRadius, CV_RGB(0, 0, 255), -1);









		}




		else if (lButtonDown)




		{




			// scribble the foreground









			circle(fgScribbleMask, Point(x, y), scribbleRadius, 255, -1);




			circle(showImg, Point(x, y), scribbleRadius, CV_RGB(255, 0, 0), -1);









			//fgScribbleMask.at<char>(y,x)=(char)255;




			// set variables using mask




			//showImg.setTo(redColorElement,fgScribbleMask);









			//showImg.at<Vec3b>(y,x)[0] = 0;




			//showImg.at<Vec3b>(y,x)[1] = 0;




			//showImg.at<Vec3b>(y,x)[2] = 255;




		}









	}














	imshow("Scribble Image", showImg);




	imshow("fg mask", fgScribbleMask);




	imshow("bg mask", bgScribbleMask);




}









// clear everything before closing




void destroyAll()




{




	// destroy all windows




	destroyWindow("Input Image");




	destroyWindow("Scribble Image");




	destroyWindow("Bin Per Pixel");




	destroyWindow("Edges");




	destroyWindow("bg mask");




	destroyWindow("fg mask");




	destroyWindow("Segmentation Mask");




	destroyWindow("Segmentation Image");









	// clear all data




	fgScribbleMask.release();




	bgScribbleMask.release();




	inputImg.release();




	showImg.release();




	showEdgesImg.release();




	binPerPixelImg.release();




	segMask.release();




	segShowImg.release();









	delete myGraph;














}









// init all images/vars




int init(char * imgFileName)




{




	// Read the file




	inputImg = imread(imgFileName, CV_LOAD_IMAGE_COLOR);




	showImg = inputImg.clone();




	segShowImg = inputImg.clone();



















	// Check for invalid input




	if (!inputImg.data)




	{




		cout << "Could not open or find the image: " << imgFileName << std::endl;




		return -1;




	}









	// this is the mask to keep the user scribbles




	fgScribbleMask.create(2, inputImg.size, CV_8UC1);




	fgScribbleMask = 0;




	bgScribbleMask.create(2, inputImg.size, CV_8UC1);




	bgScribbleMask = 0;




	segMask.create(2, inputImg.size, CV_8UC1);




	segMask = 0;




	showEdgesImg.create(2, inputImg.size, CV_32FC1);




	showEdgesImg = 0;




	binPerPixelImg.create(2, inputImg.size, CV_32F);














	// get bin index for each image pixel, store it in binPerPixelImg




	getBinPerPixel(binPerPixelImg, inputImg, numBinsPerChannel, numUsedBins);









	// compute the variance of image edges between neighbors




	getEdgeVariance(inputImg, showEdgesImg, varianceSquared);



















	// Create a window for display.




	namedWindow("Input Image", CV_WINDOW_AUTOSIZE);




	namedWindow("Scribble Image", CV_WINDOW_AUTOSIZE);




	namedWindow("Bin Per Pixel", CV_WINDOW_AUTOSIZE);




	namedWindow("Edges", CV_WINDOW_AUTOSIZE);




	namedWindow("Segmentation Mask", CV_WINDOW_AUTOSIZE);




	namedWindow("Segmentation Image", CV_WINDOW_AUTOSIZE);




	namedWindow("fg mask", CV_WINDOW_AUTOSIZE);




	namedWindow("bg mask", CV_WINDOW_AUTOSIZE);














	//namedWindow("Input Image", CV_WINDOW_NORMAL | CV_WINDOW_KEEPRATIO | CV_GUI_EXPANDED);














	// Show our image inside it.




	imshow("Input Image", inputImg);




	imshow("Scribble Image", showImg);




	imshow("Segmentation Mask", segMask);




	imshow("Segmentation Image", segShowImg);




	imshow("fg mask", fgScribbleMask);




	imshow("bg mask", bgScribbleMask);














	moveWindow("Scribble Image", 1, 1);




	moveWindow("Input Image", inputImg.cols + 50, 1);




	moveWindow("Bin Per Pixel", 2 * (inputImg.cols + 50), 1);




	moveWindow("Edges", 2 * (inputImg.cols + 55), 1);














	// set the callback on mouse




	setMouseCallback("Scribble Image", onMouse, 0);














	myGraph = new GraphType(/*estimated # of nodes*/ inputImg.rows * inputImg.cols + numUsedBins,




		/*estimated # of edges=11 spatial neighbors and one link to auxiliary*/ 12 * inputImg.rows * inputImg.cols);









	GraphType::node_id currNodeId = myGraph->add_node((int)inputImg.cols * inputImg.rows + numUsedBins);














	//#pragma omp parallel for




	for (int i = 0; i<inputImg.rows; i++)




	{




		//#pragma omp parallel for




		for (int j = 0; j<inputImg.cols; j++)




		{




			// this is the node id for the current pixel




			GraphType::node_id currNodeId = i * inputImg.cols + j;









			// add hard constraints based on scribbles




			if (fgScribbleMask.at<uchar>(i, j) == 255)




				myGraph->add_tweights(currNodeId, (int)ceil(INT32_CONST * HARD_CONSTRAINT_CONST + 0.5), 0);




			else if (bgScribbleMask.at<uchar>(i, j) == 255)




				myGraph->add_tweights(currNodeId, 0, (int)ceil(INT32_CONST * HARD_CONSTRAINT_CONST + 0.5));









			// You can now access the pixel value with cv::Vec3b




			float b = (float)inputImg.at<Vec3b>(i, j)[0];




			float g = (float)inputImg.at<Vec3b>(i, j)[1];




			float r = (float)inputImg.at<Vec3b>(i, j)[2];









			// go over the neighbors




			for (int si = -NEIGHBORHOOD; si <= NEIGHBORHOOD && si + i < inputImg.rows && si + i >= 0; si++)




			{




				for (int sj = 0; sj <= NEIGHBORHOOD && sj + j < inputImg.cols; sj++)




				{




					if ((si == 0 && sj == 0) ||




						(si == 1 && sj == 0) ||




						(si == NEIGHBORHOOD && sj == 0))




						continue;









					// this is the node id for the neighbor




					GraphType::node_id nNodeId = (i + si) * inputImg.cols + (j + sj);









					float nb = (float)inputImg.at<Vec3b>(i + si, j + sj)[0];




					float ng = (float)inputImg.at<Vec3b>(i + si, j + sj)[1];




					float nr = (float)inputImg.at<Vec3b>(i + si, j + sj)[2];









					//   ||I_p - I_q||^2  /   2 * sigma^2




					float currEdgeStrength = exp(-((b - nb)*(b - nb) + (g - ng)*(g - ng) + (r - nr)*(r - nr)) / (2 * varianceSquared));




					float currDist = sqrt((float)si*(float)si + (float)sj*(float)sj);









					// this is the edge between the current two pixels (i,j) and (i+si, j+sj)




					currEdgeStrength = ((float)0.95 * currEdgeStrength + (float)0.05) / currDist;




					myGraph->add_edge(currNodeId, nNodeId,    /* capacities */ (int)ceil(INT32_CONST*currEdgeStrength + 0.5), (int)ceil(INT32_CONST*currEdgeStrength + 0.5));









				}




			}




			// add the adge to the auxiliary node




			int currBin = (int)binPerPixelImg.at<float>(i, j);









			myGraph->add_edge(currNodeId, (GraphType::node_id)(currBin + inputImg.rows * inputImg.cols),




				/* capacities */ (int)ceil(INT32_CONST*bha_slope + 0.5), (int)ceil(INT32_CONST*bha_slope + 0.5));




		}









	}









	return 0;




}









// get bin index for each image pixel, store it in binPerPixelImg




void getBinPerPixel(Mat & binPerPixelImg, Mat & inputImg, int numBinsPerChannel, int & numUsedBins)




{




	// this vector is used to through away bins that were not used ����x��y���ݡ���ֵ64*64*64�ռ��г�ֵ����-1




	vector<int> occupiedBinNewIdx((int)pow((double)numBinsPerChannel, (double)3), -1);














	// go over the image




	int newBinIdx = 0;









	//#pragma omp parallel for




	for (int i = 0; i<inputImg.rows; i++)




	for (int j = 0; j<inputImg.cols; j++)




	{




		// You can now access the pixel value with cv::Vec3b




		float b = (float)inputImg.at<Vec3b>(i, j)[0];




		float g = (float)inputImg.at<Vec3b>(i, j)[1];




		float r = (float)inputImg.at<Vec3b>(i, j)[2];









		// this is the bin assuming all bins are present




		int bin = (int)(floor(b / 256.0 *(float)numBinsPerChannel) + (float)numBinsPerChannel * floor(g / 256.0*(float)numBinsPerChannel)




			+(float)numBinsPerChannel * (float)numBinsPerChannel * floor(r / 256.0*(float)numBinsPerChannel));














		// if we haven't seen this bin yet




		if (occupiedBinNewIdx[bin] == -1)




		{




			// mark it seen and assign it a new index




			occupiedBinNewIdx[bin] = newBinIdx;




			newBinIdx++;




		}




		// if we saw this bin already, it has the new index




		binPerPixelImg.at<float>(i, j) = (float)occupiedBinNewIdx[bin];









		//cout << bin << endl;




	}









	double maxBin;




	minMaxLoc(binPerPixelImg, NULL, &maxBin);//ͼ���е����ֵ




	numUsedBins = (int)maxBin + 1;




	imshow("Bin Per Pixel", binPerPixelImg / maxBin);









	occupiedBinNewIdx.clear();




	cout << "Num occupied bins:" << numUsedBins << endl;









}









// compute the variance(�仯������) of image edges between neighbors




void getEdgeVariance(Mat & inputImg, Mat & showEdgesImg, float & varianceSquared)




{














	varianceSquared = 0;




	int counter = 0;









#pragma omp parallel for




	for (int i = 0; i<inputImg.rows; i++)




	{




		for (int j = 0; j<inputImg.cols; j++)




		{









			// You can now access the pixel value with cv::Vec3b




			float b = (float)inputImg.at<Vec3b>(i, j)[0];




			float g = (float)inputImg.at<Vec3b>(i, j)[1];




			float r = (float)inputImg.at<Vec3b>(i, j)[2];




			for (int si = -NEIGHBORHOOD; si <= NEIGHBORHOOD && si + i < inputImg.rows && si + i >= 0; si++)




			{




				for (int sj = 0; sj <= NEIGHBORHOOD && sj + j < inputImg.cols; sj++)









				{




					if ((si == 0 && sj == 0) ||




						(si == 1 && sj == 0) ||




						(si == NEIGHBORHOOD && sj == 0))




						continue;









					float nb = (float)inputImg.at<Vec3b>(i + si, j + sj)[0];




					float ng = (float)inputImg.at<Vec3b>(i + si, j + sj)[1];




					float nr = (float)inputImg.at<Vec3b>(i + si, j + sj)[2];









					varianceSquared += (b - nb)*(b - nb) + (g - ng)*(g - ng) + (r - nr)*(r - nr);




					counter++;









				}









			}




		}




	}




	varianceSquared /= counter;









	// just for visualization




	//#pragma omp parallel for




	for (int i = 0; i<inputImg.rows; i++)




	{




		for (int j = 0; j<inputImg.cols; j++)




		{














			float edgeStrength = 0;




			// You can now access the pixel value with cv::Vec3b




			float b = (float)inputImg.at<Vec3b>(i, j)[0];




			float g = (float)inputImg.at<Vec3b>(i, j)[1];




			float r = (float)inputImg.at<Vec3b>(i, j)[2];




			for (int si = -NEIGHBORHOOD; si <= NEIGHBORHOOD && si + i < inputImg.rows && si + i >= 0; si++)




			{




				for (int sj = 0; sj <= NEIGHBORHOOD && sj + j < inputImg.cols; sj++)




				{




					if ((si == 0 && sj == 0) ||




						(si == 1 && sj == 0) ||




						(si == NEIGHBORHOOD && sj == 0))




						continue;









					float nb = (float)inputImg.at<Vec3b>(i + si, j + sj)[0];




					float ng = (float)inputImg.at<Vec3b>(i + si, j + sj)[1];




					float nr = (float)inputImg.at<Vec3b>(i + si, j + sj)[2];









					//   ||I_p - I_q||^2  /   2 * sigma^2




					float currEdgeStrength = exp(-((b - nb)*(b - nb) + (g - ng)*(g - ng) + (r - nr)*(r - nr)) / (2 * varianceSquared));




					float currDist = sqrt((float)si*(float)si + (float)sj * (float)sj);














					// this is the edge between the current two pixels (i,j) and (i+si, j+sj)




					edgeStrength = edgeStrength + ((float)0.95 * currEdgeStrength + (float)0.05) / currDist;









				}




			}




			// this is the avg edge strength for pixel (i,j) with its neighbors




			showEdgesImg.at<float>(i, j) = edgeStrength;









		}




	}









	double maxEdge;




	Point maxPoint;




	minMaxLoc(showEdgesImg, NULL, &maxEdge, NULL, &maxPoint);




	//cout << showEdgesImg.at<float>(maxPoint) << endl;




	imshow("Edges", showEdgesImg / maxEdge);









}
























/*




*******************************




Mat myMat(size(3, 3), CV_32FC2);









myMat.ptr<float>(y)[2*x]; // first channel




myMat.ptr<float>(y)[2*x+1]; // second channel




*/