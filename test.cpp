#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/background_segm.hpp>

#include <stdio.h>

#include <iostream>
#include <sstream>


using namespace cv;
using namespace std;

//global variables
Mat back;
Mat frame; //current frame
Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
Ptr<BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
//cv::BackgroundSubtractorMOG2 pMOG2;
int keyboard;

//function declarations
void processVideo(char* videoFilename);
void processImages(char* firstFrameFilename);

void searchForMovement(Mat thresholdImage, Mat &cameraFeed){
	
	bool objectDetected = false;
	Mat temp;
	thresholdImage.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours(temp,contours,hierarchy,CV_RETR_EXTERNAL,CV_CHAIN_APPROX_SIMPLE );// retrieves external contours

	/// Approximate contours to polygons + get bounding rects and circles
  vector<vector<Point> > contours_poly( contours.size() );
  vector<Rect> boundRect( contours.size() );
  vector<Point2f>center( contours.size() );
  vector<float>radius( contours.size() );
  int k=0;
  for( int i = 0; i < contours.size();i++ )
     { approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
       Rect temp;
	   temp=boundingRect( Mat(contours_poly[i]) );
	  if(temp.height>cameraFeed.rows/30 && temp.width>cameraFeed.cols/30 && temp.height>temp.width )
	   boundRect[k++] = temp;
       //minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
     }
  

  /// Draw polygonal contour + bonding rects + circles
  //Mat drawing = Mat::zeros(temp.size(), CV_8UC3 );
  for( int i = 0; i< k; i++ )
     {
       Scalar color = Scalar( 255,255,255 );
       rectangle( cameraFeed, boundRect[i].tl(), boundRect[i].br(),color, 2, 8, 0 );
       
     }
   

}





int main(int argc, char* argv[])
{

    //check for the input parameter correctness
    if(argc != 3) {
        cerr <<"Incorret input list" << endl;
        cerr <<"exiting..." << endl;
        return EXIT_FAILURE;
    }

    //create GUI windows
    namedWindow("Frame");
   // namedWindow("FG Mask MOG");
    namedWindow("Foreground");
	namedWindow("Background");
    //create Background Subtractor objects
   //NOTE HERE!!!!
    pMOG2 = new BackgroundSubtractorMOG2(50,200,true); //MOG2 approach
	pMOG2->set("detectShadows",true);
	pMOG2->set("history",1);
	pMOG2->set("nmixtures",3);

    if(strcmp(argv[1], "-vid") == 0) {
        //input data coming from a video
        processVideo(argv[2]);
    }
    
    else {
        //error in reading input parameters
        cerr <<"Please, check the input parameters." << endl;
        cerr <<"Exiting..." << endl;
        return EXIT_FAILURE;
    }
    //destroy GUI windows
    destroyAllWindows();
    return EXIT_SUCCESS;
}

void processVideo(char* videoFilename) {
    //create the capture object
    VideoCapture capture(videoFilename);
    if(!capture.isOpened()){
        //error in opening the video input
        cerr << "Unable to open video file: " << videoFilename << endl;
        exit(EXIT_FAILURE);
    }
    //read input data. ESC or 'q' for quitting
    while( (char)keyboard != 'q' && (char)keyboard != 27 ){
        //read the current frame
        if(!capture.read(frame)) {
            cerr << "Unable to read next frame." << endl;
            cerr << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }
		// pMOG2.operator ()(frame,fgMaskMOG2);
		 //pMOG2.getBackgroundImage(back);
		pMOG2->operator()(frame, fgMaskMOG2,0.001);
		pMOG2->getBackgroundImage(back);
        //get the frame number and write it on the current frame
        stringstream ss;
        rectangle(frame, cv::Point(10, 2), cv::Point(100,20),
            cv::Scalar(255,255,255), -1);
        ss << capture.get(CV_CAP_PROP_POS_FRAMES);
        string frameNumberString = ss.str();
        putText(frame, frameNumberString.c_str(), cv::Point(15, 15),
            FONT_HERSHEY_SIMPLEX, 0.5 , cv::Scalar(0,0,0));
        //show the current frame and the fg masks

		int BLUR_SIZE=3;
		
		erode(fgMaskMOG2,fgMaskMOG2,cv::Mat());
		dilate(fgMaskMOG2,fgMaskMOG2,cv::Mat());

		cv::threshold(fgMaskMOG2,fgMaskMOG2,130,255,THRESH_BINARY);
		cv::blur(fgMaskMOG2,fgMaskMOG2,cv::Size(BLUR_SIZE,BLUR_SIZE));
		cv::threshold(fgMaskMOG2,fgMaskMOG2,16,255,THRESH_BINARY);

		//cv::medianBlur(fgMaskMOG2,fgMaskMOG2,3);
        
		searchForMovement(fgMaskMOG2,frame);
		imshow("Frame", frame);
		imshow("Background",back);
       // imshow("FG Mask MOG", fgMaskMOG);
        imshow("Foreground", fgMaskMOG2);
        //get the input from the keyboard
        keyboard = waitKey( 1 );
    }
    //delete capture object
    capture.release();
}