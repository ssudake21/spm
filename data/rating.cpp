#include <opencv2/opencv.hpp>
#include <iostream>
using namespace cv;
using namespace std;
#define RROWS 10
#define RCOLS 8

void generateThetaVals(Mat& theta)
{
    /*
      theta = pinv(X' * X) * X' * y
      rating = input * theta
      Weights will be updated in more better manner as we add more data.
      We can add or remove parameters as we like.
     */


    // Placed from data/fakeprofile.txt
    // straight,cut,safety,spin,power,maxpot,maxscore,overall/rating
    float train_data[] = {100,100,100,100,100,100,100,100,
	90.2,80,83,50,0,0,0,81.3,
	65,70,50,75,0,0,0,73,
	80,80,65,67,0,0,0,70,
	55,65,50,63,0,0,0,60,
	50,50,50,50,0,0,0,50,
	35,50,20,40,0,0,0,37,
	25,25,25,25,0,0,0,25,
	50,30,20,10,0,0,0,23,
	50,20,0,0,0,0,0,15};
    
    // Collect data from training data
    Mat data = Mat(RROWS, RCOLS, CV_32FC1, train_data).clone();
    Mat X = data.colRange(Range(0, 7)).clone();
    Mat Y =  data.colRange(Range(7, 8)).clone();

    // Small blackbox to find theta values
    Mat temp1 = Mat(RROWS, 1, CV_32FC1, Scalar(1));
    hconcat(temp1, X, X);
    Mat XTranspose = X.t();
    Mat temp2 = XTranspose * X;
    Mat temp3;
    Mat pseudo_inverted;
    invert(temp2, pseudo_inverted, cv::DECOMP_SVD);
    Mat temp4 = pseudo_inverted * XTranspose;
    theta = temp4 * Y;
}

float getRating(const Mat& input, const Mat& theta)
{
    Mat temp1 = Mat(1, 1, CV_32FC1, Scalar(1));
    Mat temp2 = input.clone();
    hconcat(temp1, temp2, temp2);
    Mat result = temp2 * theta;
    return  result.at<float>(0, 0);
}

void test()
{
    Mat theta, Xin;
    generateThetaVals(theta);

    // straight,cut,safety,spin,power,maxpot,maxscore
    
    // Case 1
    float input1[] = {20, 20, 20, 20,  0, 0, 0};
    Xin = Mat(1, RCOLS - 1, CV_32FC1, input1);
    cout << "===== Case 1 ==== " << endl;
    cout << "Inputs => " << Xin << endl;
    cout << "Case 1 => " << getRating(Xin, theta) << endl;

    // Case 2
    float input2[] = {40, 30, 60, 30,  0, 0, 0};
    Xin = Mat(1, RCOLS - 1, CV_32FC1, input2);
    cout << "===== Case 2 ==== " << endl;
    cout << "Inputs => " << Xin << endl;
    cout << "Case 2 => " << getRating(Xin, theta) << endl;
}

int main()
{
    test();
    return 0;
}
