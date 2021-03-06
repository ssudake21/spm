/*!
 * \file buildprofile.cpp
 * \brief Consist methods to build user profile
 */

#include "buildprofile.hpp"
#include <cmath>
#include<cstdlib>
#include <sstream>
using namespace cv;
using namespace std;
extern float gSpin,gStraight,gCut,gOverall,gPower,gSafety;
 
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}


static double profileStraight(double angleError, double dist)
{
    const double angle_weight = 100, dist_weight = 100;
    double r_angle, r_dist;
    r_angle = (angleError * angle_weight / ANGLE_RANGE);
    r_dist = (dist * dist_weight / DISTANCE_RANGE);
    return 100 - (r_angle * r_dist)/100;
}

static double profileCut(double angleError, double dist)
{
    const double angle_weight = 110, dist_weight = 90.905;
    double r_angle, r_dist;
    r_angle = (angleError * angle_weight / ANGLE_RANGE);
    r_dist = (dist * dist_weight / DISTANCE_RANGE);
    return 100 - (r_angle * r_dist)/100;
}

static double profileSpin(double angleError, double dist)
{
    const double angle_weight = 122, dist_weight = 81.9672;
    double r_angle, r_dist;
    r_angle = (angleError * angle_weight / ANGLE_RANGE);
    r_dist = (dist * dist_weight / DISTANCE_RANGE);
    return 100 - (r_angle * r_dist)/100;
}

static double profileSafety(double angleError, double dist)
{
    const double angle_weight = 110, dist_weight = 90.905;
    double r_angle, r_dist;
    r_angle = (angleError * angle_weight / ANGLE_RANGE);
    r_dist = (dist * dist_weight / DISTANCE_RANGE);
    return 100 - (r_angle * r_dist)/100;
}

// Optimizations in sigmoid functions
static double var_sigmoid(double x)
{
    return 1 - (1.0 / (1.0 + exp(-x * 0.01)));
}

static double n_sigmoid(double x)
{
    return (1.0 / (1.0 + exp(-x * 0.1))) - 0.5;
}

// Estimate particular strenght for each shot type
// We can profile as our input range grows
static double estimateStrength(vector<double> accuracy_vals)
{
    int no_values = accuracy_vals.size();

    double sum = 0;
    for(int i = 0; i < no_values; i++)
	sum += accuracy_vals[i];
    double mean = sum / no_values;
    
    sum = 0;
    for(int i = 0; i < no_values; i++){
       sum += pow(accuracy_vals[i] - mean, 2);
    }
    
    double variance = sum / (no_values);
    
    // cout << "No => " << no_values << endl;
    // cout << "Sigmoid =>"<< n_sigmoid(no_values) << " " << var_sigmoid(sqrt(variance)) << endl;
    // cout << "Mean => " << mean << endl;

    // Combining two sigmoid functions to yeild better result
    // based on variance and no of values
    // Estimate will be always between 0 to 100
    double estimate = (n_sigmoid(no_values) + var_sigmoid(sqrt(variance))) * mean;
    
    return estimate;
}

/*!
  Profile Spec
  =============================================================================
  Profiling consist of two basic parts
  a. Ranking =>
  Just place users in ladder & find each individual's position among others.
  Create fake csv file with data input & test data.
  Data Input =>
  Input(user name, straight, cut, safety, spin, power, max pot, max score), Output(overall)
  Test Input => 
  Input(user name, straight, cut, safety, spin, power, max pot, max score), Output(overall)

  Apply Normal Equation Method to csv.
  theta = pinv(X' * X) * X' * y
  rating = input * theta

  Every time snooker program starts we should train our ranking finder with fake
  data. Then in real time we should apply to user's profile whenever above input
  data for user changes with weights we got from Normal Equation Method.

  For getting correct values for input(straight, cut, safety, ...)
  Each value should be in range 0 to 100(can be float)
  By default value should be 0.
  These values should get incremented or decremented based on users
  shot history.
  Shot classification is required for this.
  ------------------------------------------------------------------------------
  
  b. Rating => Find appropriate value for user's skills on particular scale.
  Scale should be fair to all user's
  Currently we will sort users on based overall only & rank them.
  There are many advanced cases should be considered , will take
  them after project competition.
  
  -----------------------------------------------------------------------------
 */

/*!
 *\BuildProfile constructor initialises database object,
 *\sets values of player profile to actual values from database.
 */
BuildProfile::BuildProfile(string id)
{

    string dbPath = "database/snooker.db";
    char path[dbPath.size()+1];
    dbPath.copy(path,dbPath.size(),0);
    path[dbPath.size()] = '\0';
    db = new Database(path);
    //get the values of player profile from database
    string query ="SELECT * FROM profile WHERE userID='" + id +"';";
    char temp[query.size()+1];
    query.copy(temp,query.size(),0);
    temp[query.size()] = '\0';
    vector<vector<string> > result = db->query(temp);
    for(vector<vector<string> >::iterator it = result.begin();
	it < result.end(); ++it)
    {
        vector<string> row = *it;
        straight = atof(row.at(1).c_str());
        cut = atof(row.at(2).c_str());
        safety = atof(row.at(3).c_str());
        spin = atof(row.at(4).c_str());
        powerAcc = atof(row.at(5).c_str());
	maxpot = atof(row.at(6).c_str());
	maxscore = atof(row.at(7).c_str());
        overall = atof(row.at(8).c_str());
    }
    gCut = cut;
    gSpin = spin;
    gStraight = straight;
    gPower = powerAcc;
    gOverall = overall;
    gSafety = safety;
    generateThetaVals();
}

BuildProfile::~BuildProfile()
{
    db->close();
}

/*
 *Setter for setting last Frame that is to be given to/used by
 *Shot suggestion system.
 */
void BuildProfile :: setLastFrame(Mat src)
{
    lastFrame = src;
}

Mat BuildProfile :: getLastFrame()
{
    return lastFrame;
}

void BuildProfile :: generateThetaVals()
{
    /*
      theta = pinv(X' * X) * X' * y
      rating = input * theta
      Weights will be updated in more better manner as we add more data.
      We can add or remove parameters as we like.
     */
    // Placed from data/fakeprofile.txt
    // straight,cut,safety,spin,power,maxpot,maxscore,overall/rating
    float train_data[] = {
	100, 100, 100, 100, 100, 100, 100, 100,
	100, 100, 100, 100,  0, 0, 0, 100,			      
	90.2, 80, 83, 50, 0, 0, 0, 81.3, 
	55, 60, 50, 63, 0, 0, 0, 62,
	80, 80, 65, 67, 0, 0, 0, 67, 			      
	50, 50, 50, 50, 0, 0, 0, 50, 
	35, 50, 20, 40, 0, 0, 0, 37, 
	25, 25, 25, 25, 0, 0, 0, 25, 
	50, 30, 20, 10, 0, 0, 0, 23, 
	50, 20, 0, 0, 0, 0, 0, 15,
	65, 70, 50, 75, 0, 0, 0, 73 			      
    };

    
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
    //cout << "Inntial Theta Values" << endl;
    //cout << theta;
}

float BuildProfile::getRating(const Mat &input)
{
    // Find overall rating using input args
    Mat temp1 = Mat(1, 1, CV_32FC1, Scalar(1));
    Mat temp2 = input.clone();
    hconcat(temp1, temp2, temp2);
    Mat result = temp2 * theta;
    return  result.at<float>(0, 0);
}

void BuildProfile :: build(string userID,int shottype)
{
    // Update Respective accuracy for shotype
    // Parse all from database & apply some statistical method
    string query = "select accuracy from shothistory where userId='" 
	+ patch::to_string(userID)
	+"' and shottype="
	+ patch::to_string(shottype)
	+ ";";
    char temp[query.size()+1];
    query.copy(temp,query.size(),0);
    temp[query.size()] = '\0';
    vector<vector<string> > result = db->query(temp);

    vector<double> accuracy_vals;
    for(vector<vector<string> >::iterator it = result.begin();
     	it < result.end(); ++it)
     {
         vector<string> row = *it;
	 accuracy_vals.push_back(atof(row.at(0).c_str()));
    }

    double estimate = estimateStrength(accuracy_vals);
    
    switch(shottype)
    {
    case 1:
	straight = estimate;
	break;
    case 2:
	cut = estimate;
	break;
    case 3:
	spin = estimate;
	break;
    case 4:
	safety = estimate;
	break;
    default:
	cout << "Case not found." << endl;
	break;
    }
    
    // straight,cut,safety,spin,power,maxpot,maxscore
    float profile_input[] = {straight, cut, safety, spin, powerAcc,
			     maxpot, maxscore};
    Mat Xin = Mat(1, RCOLS - 1, CV_32FC1, profile_input);
    overall = getRating(Xin);

    query = "update profile set straight= " 
	+ patch::to_string(straight)
	+ ", cut= " + patch::to_string(cut)
	+ ", safety= " + patch::to_string(safety)
	+ ",spin= " + patch::to_string(spin)
	+ ",power= "+ patch::to_string(powerAcc)
	+ ",overall = "+ patch::to_string(overall)
	+ " where userID='" + patch::to_string(userID)
	+ "';";
    char temp2[query.size()+1];
    query.copy(temp2,query.size(),0);
    temp2[query.size()] = '\0';
    db->query(temp2);
    gCut = cut;
    gSpin = spin;
    gStraight = straight;
    gPower = powerAcc;
    gOverall = overall;
    gSafety = safety;
}

void BuildProfile :: addCurrent(string userID, double angleerror,
				double totaldist,double suggdist,
				double totaltime, double velocity,
				int shottype){
    double (*accuracy_func)(double, double), accuracy;

    // Converting shottype to straight for profiling
    // if it is unrecognized.
    if(shottype < 1)
	shottype = 1;
    
    switch(shottype)
    {
        //if straight shot
        case 1: accuracy_func = profileStraight;
	    break;
        case 2: accuracy_func = profileCut;
	    break;
        case 3: accuracy_func = profileSpin;
	    break;
        case 4: accuracy_func = profileSafety;
	    break;
        default:
	    cout << "[shot::build] No matching shot type"
		 << shottype << endl;
	    accuracy_func = profileStraight;
	    break;
    }
    accuracy = (*accuracy_func)(angleerror, suggdist);

    string query = "insert into shothistory (userID,angleerror,totaldist,suggdist,totaltime,velocity,shottype,accuracy) values('" 
        + patch::to_string(userID) 
        + "','" + patch::to_string(angleerror)
	 + "','" + patch::to_string(totaldist)
	 + "','" + patch::to_string(suggdist)
	 + "','" + patch::to_string(totaltime)
	 + "','" + patch::to_string(velocity)
	 + "','" + patch::to_string(shottype)
	 + "','" + patch::to_string(accuracy)
	 + "');";
    char temp[query.size()+1];
    query.copy(temp,query.size(),0);
    temp[query.size()] = '\0';
    db->query(temp);
    build(userID, shottype);
}
