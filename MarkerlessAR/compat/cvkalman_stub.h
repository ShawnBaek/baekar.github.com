// CvKalman stub for OpenCV 4.x compatibility
// OpenCV 4 removed the C Kalman filter API (CvKalman, cvCreateKalman, etc.)
// This stub provides the struct and function declarations so existing code compiles.
// The Kalman filter functionality will need proper migration in Sprint 2.

#ifndef CVKALMAN_STUB_H
#define CVKALMAN_STUB_H

#include <opencv2/core/core_c.h>
#include <cstdlib>
#include <cstring>

typedef struct CvKalman {
    int MP;                     // number of measurement parameters
    int DP;                     // number of dynamic parameters
    int CP;                     // number of control parameters

    CvMat* state_pre;           // predicted state (x'(k)): x(k)=A*x(k-1)+B*u(k)
    CvMat* state_post;          // corrected state (x(k)): x(k)=x'(k)+K(k)*(z(k)-H*x'(k))
    CvMat* transition_matrix;   // state transition matrix (A)
    CvMat* measurement_matrix;  // measurement matrix (H)
    CvMat* process_noise_cov;   // process noise covariance matrix (Q)
    CvMat* measurement_noise_cov; // measurement noise covariance matrix (R)
    CvMat* error_cov_pre;       // priori error estimate covariance matrix (P'(k))
    CvMat* error_cov_post;      // posteriori error estimate covariance matrix (P(k))
    CvMat* gain;                // Kalman gain matrix (K(k))
    CvMat* temp1;
    CvMat* temp2;
    CvMat* temp3;
    CvMat* temp4;
    CvMat* temp5;
} CvKalman;

static inline CvKalman* cvCreateKalman(int dynam_params, int measure_params, int control_params) {
    CvKalman* kalman = (CvKalman*)calloc(1, sizeof(CvKalman));
    kalman->DP = dynam_params;
    kalman->MP = measure_params;
    kalman->CP = control_params;

    kalman->state_pre = cvCreateMat(dynam_params, 1, CV_32FC1);
    kalman->state_post = cvCreateMat(dynam_params, 1, CV_32FC1);
    kalman->transition_matrix = cvCreateMat(dynam_params, dynam_params, CV_32FC1);
    kalman->measurement_matrix = cvCreateMat(measure_params, dynam_params, CV_32FC1);
    kalman->process_noise_cov = cvCreateMat(dynam_params, dynam_params, CV_32FC1);
    kalman->measurement_noise_cov = cvCreateMat(measure_params, measure_params, CV_32FC1);
    kalman->error_cov_pre = cvCreateMat(dynam_params, dynam_params, CV_32FC1);
    kalman->error_cov_post = cvCreateMat(dynam_params, dynam_params, CV_32FC1);
    kalman->gain = cvCreateMat(dynam_params, measure_params, CV_32FC1);
    kalman->temp1 = cvCreateMat(dynam_params, dynam_params, CV_32FC1);
    kalman->temp2 = cvCreateMat(measure_params, dynam_params, CV_32FC1);
    kalman->temp3 = cvCreateMat(measure_params, measure_params, CV_32FC1);
    // temp4 holds P_pre * H^T (DPxDP * DPxMP = DPxMP), used as K's first
    // operand in the gain computation. Must be DPxMP, not MPxDP — the
    // wrong shape tripped cvGEMM at runtime ("D.rows == A.rows").
    kalman->temp4 = cvCreateMat(dynam_params, measure_params, CV_32FC1);
    kalman->temp5 = cvCreateMat(measure_params, 1, CV_32FC1);

    cvSetIdentity(kalman->transition_matrix);
    cvSetIdentity(kalman->measurement_matrix);
    cvSetIdentity(kalman->process_noise_cov);
    cvSetIdentity(kalman->measurement_noise_cov);
    cvSetIdentity(kalman->error_cov_post);
    cvZero(kalman->state_post);

    return kalman;
}

static inline void cvReleaseKalman(CvKalman** kalman) {
    if (kalman && *kalman) {
        cvReleaseMat(&(*kalman)->state_pre);
        cvReleaseMat(&(*kalman)->state_post);
        cvReleaseMat(&(*kalman)->transition_matrix);
        cvReleaseMat(&(*kalman)->measurement_matrix);
        cvReleaseMat(&(*kalman)->process_noise_cov);
        cvReleaseMat(&(*kalman)->measurement_noise_cov);
        cvReleaseMat(&(*kalman)->error_cov_pre);
        cvReleaseMat(&(*kalman)->error_cov_post);
        cvReleaseMat(&(*kalman)->gain);
        cvReleaseMat(&(*kalman)->temp1);
        cvReleaseMat(&(*kalman)->temp2);
        cvReleaseMat(&(*kalman)->temp3);
        cvReleaseMat(&(*kalman)->temp4);
        cvReleaseMat(&(*kalman)->temp5);
        free(*kalman);
        *kalman = NULL;
    }
}

// Simplified Kalman predict: x'(k) = A * x(k-1)
static inline const CvMat* cvKalmanPredict(CvKalman* kalman, const CvMat* control = NULL) {
    (void)control;
    // x'(k) = A * x(k-1)
    cvMatMulAdd(kalman->transition_matrix, kalman->state_post, NULL, kalman->state_pre);
    // P'(k) = A * P(k-1) * At + Q
    cvGEMM(kalman->transition_matrix, kalman->error_cov_post, 1,
           NULL, 0, kalman->temp1, 0);
    cvGEMM(kalman->temp1, kalman->transition_matrix, 1,
           kalman->process_noise_cov, 1, kalman->error_cov_pre, CV_GEMM_B_T);
    cvCopy(kalman->state_pre, kalman->state_post);
    return kalman->state_pre;
}

// Simplified Kalman correct: x(k) = x'(k) + K(k) * (z(k) - H*x'(k))
static inline const CvMat* cvKalmanCorrect(CvKalman* kalman, const CvMat* measurement) {
    // temp2 = H * P'(k)
    cvGEMM(kalman->measurement_matrix, kalman->error_cov_pre, 1,
           NULL, 0, kalman->temp2, 0);
    // temp3 = H * P'(k) * Ht + R
    cvGEMM(kalman->temp2, kalman->measurement_matrix, 1,
           kalman->measurement_noise_cov, 1, kalman->temp3, CV_GEMM_B_T);
    // temp3 = inv(temp3)
    cvInvert(kalman->temp3, kalman->temp3, CV_SVD);
    // K(k) = P'(k) * Ht * inv(temp3)
    cvGEMM(kalman->error_cov_pre, kalman->measurement_matrix, 1,
           NULL, 0, kalman->temp4, CV_GEMM_B_T);
    cvMatMul(kalman->temp4, kalman->temp3, kalman->gain);
    // temp5 = z(k) - H * x'(k)
    cvGEMM(kalman->measurement_matrix, kalman->state_pre, -1,
           measurement, 1, kalman->temp5, 0);
    // x(k) = x'(k) + K(k) * temp5
    cvGEMM(kalman->gain, kalman->temp5, 1,
           kalman->state_pre, 1, kalman->state_post, 0);
    // P(k) = (I - K(k)*H) * P'(k)
    cvGEMM(kalman->gain, kalman->measurement_matrix, -1,
           NULL, 0, kalman->temp1, 0);
    // Add identity
    for (int i = 0; i < kalman->DP; i++)
        CV_MAT_ELEM(*kalman->temp1, float, i, i) += 1.0f;
    cvMatMul(kalman->temp1, kalman->error_cov_pre, kalman->error_cov_post);

    return kalman->state_post;
}

// CvRandState stub — removed from OpenCV 4
// Used by the Kalman filter code for generating process noise
typedef struct CvRandState {
    uint64 state;     // RNG state
    int    disttype;  // distribution type (0=uniform, 1=normal)
    CvScalar param[2]; // distribution parameters
} CvRandState;

static inline void cvRandSetRange(CvRandState* state, double param1, double param2, int /*index*/) {
    (void)state;
    (void)param1;
    (void)param2;
}

static inline void cvRand(CvRandState* state, CvArr* arr) {
    (void)state;
    // Fill with zeros as a stub — proper RNG migration in Sprint 2
    if (arr) {
        CvMat stub;
        CvMat* mat = cvGetMat(arr, &stub);
        cvSetZero(mat);
    }
}

#endif // CVKALMAN_STUB_H
