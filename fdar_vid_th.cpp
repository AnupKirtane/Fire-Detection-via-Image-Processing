/*Fire Detection via Image processing using Multithreading*/
#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <time.h>
#include <thread>
#include <chrono>
using namespace std::chrono;
using namespace std;
using namespace cv;

/*void check_program_arguments(int argc) {
	if(argc != 2) {
		std::cout << "Error! Program usage:" << std::endl;
		std::cout << "fire01.jpg" << std::endl;
		std::exit(-1);
	}
}

void check_if_image_exist(const cv::Mat &img, const std::string &path) {
	if(img.empty()) {
		std::cout << "Error! Unable to load image: " << path << std::endl;
		std::exit(-1);
	}
}*/

//Red pixel threshold calculation
int calculateRThreshold(cv::Mat image){
	int r_avg = 0;
	int countPixels = 0;
	for(int i = 0; i < image.rows; i++){

		for(int j = 0; j < image.cols; j++){

			Vec3b color = image.at<Vec3b>(i,j);
			r_avg += color[2];
			++countPixels;
		}
	}
	return r_avg /= countPixels;
}

//Saturation threshold calculation
int calculateSThreshold(Mat image, Mat hsv_im, int r_thresh){
        int s_avg = 0;
        int countSatPixels = 0;
	
        for(int i = 0; i < image.rows; i++){

                for(int j = 0; j < image.cols; j++){

                        Vec3b color = image.at<Vec3b>(i,j);
			Vec3b hsv = hsv_im.at<Vec3b>(i,j);

			if( color[2] > r_thresh){

                        	s_avg += hsv[1];
                        	++countSatPixels;
			}
                }
        }

        return (s_avg /= countSatPixels )+ 90;
}

/*Applying the prime 3 rules for fire detection
1. R > RThreshold
2. R >= G  >  B
3. S >= ( (255 - R) * SThreshold  / RThreshold  ) */
Mat applyFireDetectionRules( cv::Mat bgr_image, cv::Mat hsv_image, int r_thresh, int s_thresh){
	cv::Mat proc_image;
	bgr_image.copyTo(proc_image);

	for(int i = 0; i < bgr_image.rows; i++){

                for(int j = 0; j < bgr_image.cols; j++){

                        Vec3b bgr_vec = bgr_image.at<Vec3b>(i,j);
			Vec3b hsv_vec = hsv_image.at<Vec3b>(i,j);

			int redPix = bgr_vec[2];
			int satPix = hsv_vec[1];
			if( (redPix > r_thresh) && ( (0<=hsv_vec[0]) && (hsv_vec[0] <=30) ) 
				&& (satPix >= ( (255 - redPix) * s_thresh/r_thresh) ) ){

				bgr_vec[0] = 255;
				bgr_vec[1] =  255;
				bgr_vec[2] = 255;

				proc_image.at<Vec3b>(i,j) = bgr_vec;
			}
			else{
               			bgr_vec[0] = 0;
                                bgr_vec[1] =  0;
                                bgr_vec[2] = 0;
                                proc_image.at<Vec3b>(i,j) = bgr_vec;
			}
        	}
	}
	return proc_image;

}

//Thread 1 function
void calculateThresholds(Mat &bgr_image, int & Rth){

	Rth = calculateRThreshold(bgr_image);


}

//Thread 2 function
void getHSVImage(Mat &bgr_image, Mat &hsv_image){

	cv::cvtColor(bgr_image, hsv_image, cv::COLOR_BGR2HSV);   	//convert colorspace of image from BGR to HSV

}



//Main thread
int main(int argc, char **argv) {
	Mat bgr_image, proc_image, hsv_image, prev_proc_image, diff_image;  	//Mat = Matrix data type for images
	int Rth=0, Sth=0;

	high_resolution_clock::time_point time1;
	high_resolution_clock::time_point time2;
	
	time1 = high_resolution_clock::now();  	//initiate time1 ticking
	VideoCapture cap(CV_CAP_ANY);         	// open the default camera
	if(!cap.isOpened())                  	// check if we succeeded
        	return -1;
	
	
	double duration;
	double avg_time = 0;
	
	time2 = high_resolution_clock::now();  //initiate time2 ticking
		
	duration = duration_cast<microseconds>( time2 - time1 ).count(); //calculate duration for lines executed between time1 and time2 initiation

	// SETUP test here first.....................................

	time1 = high_resolution_clock::now();   //initiate time1 ticking
	cap >> bgr_image;                     	//get a new frame from camera
	bgr_image.copyTo(hsv_image);			//copying BGR image matrix to HSV image matrix
				
	// calculating condition, R threshold , S threshold
	Rth = calculateRThreshold(bgr_image);
	Sth = calculateSThreshold(bgr_image,hsv_image, Rth);

	// Convert input image to HSV
	cv::cvtColor(bgr_image, hsv_image, cv::COLOR_BGR2HSV);					//convert colorspace of BGR image to HSV 
	prev_proc_image = applyFireDetectionRules(bgr_image,hsv_image,Rth,Sth); //Applying fire detction rules to image frame

	time2 = high_resolution_clock::now(); //initiate time2 ticking
		
	duration = duration_cast<microseconds>( time2 - time1 ).count(); //calculate duration for lines executed between time1 and time2 initiation
	
	cout << "Starting Test..." << endl; 		
	//cout << "Total execution time before loop: " << duration << endl; 
	

	double max_response_time = 0;
	double min_response_time = 10000000;
	int i;
	thread t1;   	//declaring thread1
	thread t2; 		//declaring thread2	
	for(i = 0; i < 100; i++){                                       //for loop initiated for first 100 frames
		
		
		time1 = high_resolution_clock::now();  	//initiate time1 ticking 
        cap >> bgr_image;   					//get a new frame from camera
		Rth=0, Sth=0;	
		
		// calculating condition, R threshold
		t1 = thread(calculateThresholds, ref(bgr_image),ref(Rth)); 		//calling thread t1
		t2 = thread(getHSVImage, ref(bgr_image), ref(hsv_image) );		//calling thread t2

		t1.join();		//joins / synchronizes t1 with main thread 
		t2.join();		//joins / synchronizes t2 with main thread
		
		Sth = calculateSThreshold(bgr_image, hsv_image, Rth);   		// Calculate S_threshold
		
		proc_image = applyFireDetectionRules(bgr_image,hsv_image,Rth,Sth);	//Applying fire detection rules to image frame

		diff_image = proc_image - prev_proc_image; //differencing out the static pixels by comparing two different frames
		
  		erode(diff_image,diff_image, getStructuringElement(MORPH_RECT, Size(3, 3)) ); //erode() to reduce the noise pixels
		
		prev_proc_image = proc_image;				//store processed frame as previously processed image frame
		time2 = high_resolution_clock::now();		//initiate time2 ticking
	
		duration = duration_cast<microseconds>( time2 - time1 ).count(); //calculate duration for lines executed between time1 and time2 initiation
		avg_time += duration; 

        //cout <<"Total execution time: " << duration<<endl;
		imshow("Difference Frame", diff_image);		//window showing output for diff_image
		cv::imshow("FDAR Frame", proc_image);		//window showing output image after applying fire detection rules
		if( duration > max_response_time){
			max_response_time = duration;
		}
		if( duration < min_response_time){			
			min_response_time = duration;
		}

		if(waitKey(30) >= 0) break;
		
    	}

	t1.~thread();									//kill thread t1
	t2.~thread();									//kill thread t2
	
	cout << "Result for " << i << " Frame Iterations"<< endl;
	cout << "Average time is: " << avg_time/100<< " us" << endl;
  	cout << "Maximum response time is: " << max_response_time << " us" << endl; 
	cout << "Minimum response time is: " << min_response_time << " us" << endl;
	cout << "...Test Ended" << endl; 
  	
	return 0;
}
