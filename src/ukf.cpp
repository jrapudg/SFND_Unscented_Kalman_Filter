#include <iostream>
#include "ukf.h"
#include "Eigen/Dense"

using Eigen::MatrixXd;
using Eigen::VectorXd;

/**
 * Initializes Unscented Kalman filter
 */
UKF::UKF() {
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // State dimension
  n_x_ = 5;

  // Augmented state dimension
  n_aug_  = 7;

  // Sigma point spreading parameter
  lambda_ = 3;

  // initial state vector
  x_ = VectorXd(n_x_);

  // initial covariance matrix
  P_ = MatrixXd::Identity(n_x_, n_x_);

  // Lidar Measurement Model
  H_ = MatrixXd(2, 5);
  H_ << 1, 0, 0, 0, 0,
            0, 1, 0, 0, 0;
  
  /*
  These values of std_a_ and std_yawdd_ are too high. Imagine with 5 m/s^2, 
  and 8 m/s^2 accelerations in real life. It's not good for the human comfort 
  level. These values should be between 0 and 3.And when you change these values 
  to that range, then you might need to fine-tune your covariance matrix P_. 
  Possibly the last two elements of the diagonal of the identity matrix with 
  the square of the standard deviation of laser or radar measurement noise.
  */

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 5; //5

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 8; //8
  
  /**
   * DO NOT MODIFY measurement noise values below.
   * These are provided by the sensor manufacturer.
   */

  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;

  R_lidar_ = MatrixXd(2, 2);
  R_lidar_ << std_laspx_, 0,
            0, std_laspy_;

  is_initialized_ = false;

  MatrixXd Xsig_pred_ = MatrixXd(n_aug_, 2 * n_aug_ + 1);
  // add measurement noise covariance matrix
  R_radar_ = MatrixXd(3,3);
  R_radar_ <<  std_radr_*std_radr_, 0, 0,
        0, std_radphi_*std_radphi_, 0,
        0, 0,std_radrd_*std_radrd_;

  // create vector for weights
  weights_ = VectorXd(2*n_aug_+1);

  for (int i = 0; i < 2*n_aug_ + 1; ++i){
    if (i == 0){
        weights_(i) = lambda_/(lambda_+n_aug_);
    }
    else{
        weights_(i) = 1/(2*(lambda_+n_aug_));
    }
  }
  /**
   * End DO NOT MODIFY section for measurement noise values 
   */

}

UKF::~UKF() {}

void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
   * TODO: Complete this function! Make sure you switch between lidar and radar
   * measurements.
   */
  if (!is_initialized_) {
    //cout << "Kalman Filter Initialization " << endl;

    if (meas_package.sensor_type_ == MeasurementPackage::LASER){
      // set the state with the initial location and zero velocity
      x_ << meas_package.raw_measurements_[0], 
                meas_package.raw_measurements_[1], 
                0, 
                0,
                0;
      time_us_ = meas_package.timestamp_;
      is_initialized_ = true;
      std::cout<< "LIDAR " << std::endl;
      std::cout << "x: "<< x_ << std::endl;
  std::cout << "P: "<< P_ << std::endl;
    }
    else if (meas_package.sensor_type_ == MeasurementPackage::RADAR){
      x_ << meas_package.raw_measurements_[0]*cos(meas_package.raw_measurements_[1]), 
                meas_package.raw_measurements_[0]*sin(meas_package.raw_measurements_[1]), 
                meas_package.raw_measurements_[2], 
                meas_package.raw_measurements_[1],
                0;
      time_us_ = meas_package.timestamp_;
      is_initialized_ = true;
      std::cout<< "RADAR " << std::endl;
      std::cout << "x: "<< x_ << std::endl;
  std::cout << "P: "<< P_ << std::endl;

    }
      return;    
  }

  // compute the time elapsed between the current and previous measurements
  // dt - expressed in seconds
  double delta_t = (meas_package.timestamp_ - time_us_ ) / 1000000.0;
  time_us_  = meas_package.timestamp_;
   std::cout<< "dt = " << delta_t << std::endl;

  if ((meas_package.sensor_type_ == MeasurementPackage::RADAR) && (use_radar_)){
    //if (delta_t > 0.0001){
      UKF::Prediction(delta_t);
    //}
    UKF::UpdateRadar(meas_package);
  }
  else if ((meas_package.sensor_type_ == MeasurementPackage::LASER) && (use_laser_)){
    //if (delta_t > 0.0001){
      UKF::Prediction(delta_t);
    //}
    UKF::UpdateLidar(meas_package);
  }
  else
    return;
}


void UKF::Prediction(double delta_t) {
   std::cout << "Prediction init" << std::endl;
  /**
   * TODO: Complete this function! Estimate the object's location. 
   * Modify the state vector, x_. Predict sigma points, the state, 
   * and the state covariance matrix.
   */
   UKF::GenerateSigmaPoints();

   MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

   UKF::AugmentedSigmaPoints(&Xsig_aug);

   UKF::SigmaPointPrediction(Xsig_aug, delta_t);

   UKF::PredictMeanAndCovariance();
   std::cout << "Prediction end" << std::endl;
}

void UKF::UpdateLidar(MeasurementPackage meas_package) {
  std::cout << "UpdateLidar init" << std::endl;
  /*
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the lidar NIS, if desired.
   */
  VectorXd z = meas_package.raw_measurements_;
  VectorXd z_pred = H_ * x_;
  VectorXd y = z - z_pred;
  MatrixXd Ht = H_.transpose();
  MatrixXd S = H_ * P_ * Ht + R_lidar_;
  MatrixXd Si = S.inverse();
  MatrixXd PHt = P_ * Ht;
  MatrixXd K = PHt * Si;

  //new estimate
  x_ = x_ + (K * y);
  long x_size = x_.size();
  MatrixXd I = MatrixXd::Identity(x_size, x_size);
  P_ = (I - K * H_) * P_;
  std::cout << "x: "<< x_ << std::endl;
  std::cout << "P: "<< P_ << std::endl;
  std::cout << "UpdateRadar end" << std::endl;

}

void UKF::UpdateRadar(MeasurementPackage meas_package) {
  std::cout << "UpdateRadar init" << std::endl;
  /**
   * TODO: Complete this function! Use radar data to update the belief 
   * about the object's position. Modify the state vector, x_, and 
   * covariance, P_.
   * You can also calculate the radar NIS, if desired.
   */
   // mean predicted measurement
  VectorXd z_pred = VectorXd(3);
  
  // measurement covariance matrix S
  MatrixXd S = MatrixXd(3, 3);

  MatrixXd Zsig = MatrixXd(3, 2 * n_aug_ + 1);

  UKF::PredictRadarMeasurement(&z_pred, &S, &Zsig);

  VectorXd z = meas_package.raw_measurements_;

  //std::cout << "Before" << std::endl;
  UKF::UpdateStateRadar(z, z_pred, S, Zsig); 

  std::cout << "UpdateRadar end" << std::endl; 

}

void UKF::GenerateSigmaPoints() {
  std::cout << "GenerateSigmaPoints init" << std::endl;
  // create sigma point matrix
  MatrixXd Xsig = MatrixXd(n_x_, 2 * n_x_ + 1);

  // calculate square root of P
  MatrixXd A = P_.llt().matrixL();
  Xsig.col(0) << x_;
  // set sigma points as columns of matrix Xsig
  
  for (int i = 0; i < n_x_; ++i){
      Xsig.col(i+1) = x_+ A.col(i)*sqrt(lambda_ + n_x_);
      Xsig.col(i+n_x_+1) = x_ - A.col(i)*sqrt(lambda_ + n_x_);
  }
  Xsig_pred_ = Xsig;
  std::cout << "Xsig: "<< Xsig_pred_ << std::endl;
  std::cout << "GenerateSigmaPoints end" << std::endl;
}


void UKF::AugmentedSigmaPoints(MatrixXd* Xsig_out) {
  std::cout << "AugmentedSigmaPoints init" << std::endl;
  // create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  // create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);

  // create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);
 
  // create augmented mean state
  x_aug.head(n_x_) = x_;
  x_aug(n_x_) = 0;
  x_aug(6) = 0;

  // create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(n_x_, n_x_) = P_;
  P_aug(5,5) = std_a_*std_a_;
  P_aug(6,6) = std_yawdd_*std_yawdd_;

  // create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  // create augmented sigma points
  Xsig_aug.col(0)  = x_aug;
  for (int i = 0; i< n_aug_; ++i) {
    Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
  }
  // write result
  *Xsig_out = Xsig_aug;
  std::cout << "AugmentedSigmaPoints end" << std::endl;
}

void UKF::SigmaPointPrediction(MatrixXd& Xsig_aug, double delta_t){
  std::cout << "SigmaPointPrediction init" << std::endl;

  // create matrix with predicted sigma points as columns
  MatrixXd Xsig_pred = MatrixXd(n_x_, 2 * n_aug_ + 1);

  // predict sigma points
  for (int i = 0; i< 2*n_aug_+1; ++i) {
    // extract values for better readability
    double p_x = Xsig_aug(0,i);
    double p_y = Xsig_aug(1,i);
    double v = Xsig_aug(2,i);
    double yaw = Xsig_aug(3,i);
    double yawd = Xsig_aug(4,i);
    double nu_a = Xsig_aug(5,i);
    double nu_yawdd = Xsig_aug(6,i);

    // predicted state values
    double px_p, py_p;

    // avoid division by zero
    if (fabs(yawd) > 0.001) {
        px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
        py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
    } else {
        px_p = p_x + v*delta_t*cos(yaw);
        py_p = p_y + v*delta_t*sin(yaw);
    }

    double v_p = v;
    double yaw_p = yaw + yawd*delta_t;
    double yawd_p = yawd;

    // add noise
    px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
    py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
    v_p = v_p + nu_a*delta_t;

    yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
    yawd_p = yawd_p + nu_yawdd*delta_t;
  
    // write predicted sigma point into right column
    Xsig_pred(0,i) = px_p;
    Xsig_pred(1,i) = py_p;
    Xsig_pred(2,i) = v_p;
    Xsig_pred(3,i) = yaw_p;
    Xsig_pred(4,i) = yawd_p;
  }
  Xsig_pred_ = Xsig_pred;
  
  std::cout << "Xsig: "<< Xsig_pred_ << std::endl;
  std::cout << "SigmaPointPrediction end" << std::endl;
}

void UKF::PredictMeanAndCovariance() {
  std::cout << "PredictMeanAndCovariance init" << std::endl;
  // create vector for predicted state
  VectorXd x = VectorXd(n_x_);

  // create covariance matrix for prediction
  MatrixXd P = MatrixXd(n_x_, n_x_);
  //std::cout << "1" << std::endl;
  //std::cout << "2" << std::endl;
  // predict state mean
    // predicted state mean
  x.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // iterate over sigma points
    x = x + weights_(i) * Xsig_pred_.col(i);
  }
  //std::cout << "3" << std::endl;
  
  // predict state covariance matrix
  P.fill(0.0);
  //std::cout << "3.1" << std::endl;
  for (int i = 0; i < 2*n_aug_+1; ++i){
      VectorXd x_diff = Xsig_pred_.col(i) - x;
      //std::cout << "3.2" << std::endl;
      // angle normalization
      while (x_diff(3)> M_PI){
         //std::cout << x_diff(3) << std::endl;
         x_diff(3)-=2.*M_PI;
      }
      while (x_diff(3)<-M_PI){
        //std::cout << x_diff(3) << std::endl;
        x_diff(3)+=2.*M_PI;
      } 
      //std::cout << "3.3" << std::endl;
      P = P + weights_(i)*x_diff*x_diff.transpose();
      //std::cout << "3.4" << std::endl;
  }
  //std::cout << "4" << std::endl;

  // write result
  x_ = x;
  P_ = P;
  std::cout << "x: "<< x_ << std::endl;
  std::cout << "P: "<< P_ << std::endl;
  std::cout << "PredictMeanAndCovariance end" << std::endl;
}

void UKF::PredictRadarMeasurement(VectorXd* z_out, MatrixXd* S_out, MatrixXd* Zsig_out) {
  std::cout << "PredictRadarMeaurement init" << std::endl;

  int n_z = 3;
  // create matrix for sigma points in measurement space
  MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);

  // mean predicted measurement
  VectorXd z_pred = VectorXd(n_z);
  
  // measurement covariance matrix S
  MatrixXd S = MatrixXd(n_z, n_z);

  // transform sigma points into measurement space
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // extract values for better readability
    double p_x = Xsig_pred_(0,i);
    double p_y = Xsig_pred_(1,i);
    double v  = Xsig_pred_(2,i);
    double yaw = Xsig_pred_(3,i);

    double v1 = cos(yaw)*v;
    double v2 = sin(yaw)*v;

    // measurement model
    Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                       // r
    Zsig(1,i) = atan2(p_y,p_x);                                // phi
    Zsig(2,i) = (p_x*v1 + p_y*v2) / sqrt(p_x*p_x + p_y*p_y);   // r_dot
  }

  // mean predicted measurement
  z_pred.fill(0.0);
  for (int i=0; i < 2*n_aug_+1; ++i) {
    z_pred = z_pred + weights_(i) * Zsig.col(i);
  }

  // innovation covariance matrix S
  S.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;

    // angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    S = S + weights_(i) * z_diff * z_diff.transpose();
  }
  
  S = S + R_radar_;

  // write result
  *z_out = z_pred;
  *S_out = S;
  *Zsig_out = Zsig;
  std::cout << "z_pred: "<< z_pred << std::endl;
  std::cout << "S: "<< S << std::endl;

  std::cout << "PredictRadarMeaurement end" << std::endl;
}

void UKF::UpdateStateRadar(VectorXd &z, VectorXd &z_pred, MatrixXd &S, MatrixXd &Zsig) {
  std::cout << "Update State init" << std::endl;
  std::cout << "z: " << z << std::endl;
  // create matrix for cross correlation Tc
  MatrixXd Tc = MatrixXd(n_x_, 3);

  // calculate cross correlation matrix
  Tc.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; ++i) {  // 2n+1 simga points
    // residual
    VectorXd z_diff = Zsig.col(i) - z_pred;
    // angle normalization
    while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
    while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    // angle normalization
    while (x_diff(3)> M_PI) x_diff(3)-=2.*M_PI;
    while (x_diff(3)<-M_PI) x_diff(3)+=2.*M_PI;

    Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
  }

  // Kalman gain K;
  MatrixXd K = Tc * S.inverse();

  // residual
  VectorXd z_diff = z - z_pred;

  // angle normalization
  while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
  while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;


  // update state mean and covariance matrix
  x_ = x_ + K*z_diff;
  P_ = P_ - K*S*K.transpose();
  std::cout << "x: "<< x_ << std::endl;
  std::cout << "P: "<< P_ << std::endl;
  std::cout << "Update State end" << std::endl;
} 