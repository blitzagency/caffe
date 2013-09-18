#include <mkl.h>
#include <cublas_v2.h>

#include "caffeine/blob.hpp"
#include "caffeine/common.hpp"
#include "caffeine/filler.hpp"
#include "caffeine/layer.hpp"
#include "caffeine/vision_layers.hpp"
#include "caffeine/util/blas.hpp"

namespace caffeine {

template <typename Dtype>
void InnerProductLayer<Dtype>::SetUp(const vector<Blob<Dtype>*>& bottom,
      vector<Blob<Dtype>*>* top) {
  CHECK_EQ(bottom.size(), 1) << "IP Layer takes a single blob as input.";
  CHECK_EQ(top->size(), 1) << "IP Layer takes a single blob as output.";
  const int num_output = this->layer_param_.num_output();
  biasterm_ = this->layer_param_.biasterm();
  // Figure out the dimensions
  M_ = bottom[0]->num();
  K_ = bottom[0]->count() / bottom[0]->num();
  N_ = num_output;
  (*top)[0]->Reshape(bottom[0]->num(), num_output, 1, 1);
  if (biasterm_) {
    this->blobs_.resize(2);
  } else {
    this->blobs_.resize(1);
  }
  // Intialize the weight
  this->blobs_[0].Reshape(1, 1, K_, N_);
  // fill the weights
  shared_ptr<Filler<Dtype> > weight_filler(
      GetFiller<Dtype>(this->layer_param_.weight_filler()));
  weight_filler->Fill(&this->blobs_[0]);
  // If necessary, intiialize and fill the bias term
  if (biasterm_) {
    this->blobs_[1].Reshape(1, 1, 1, N_);
    shared_ptr<Filler<Dtype> > bias_filler(
        GetFiller<Dtype>(this->layer_param_.bias_filler()));
    bias_filler->Fill(&this->blobs_[1]);
    bias_multiplier_.reset(new SyncedMemory(M_ * sizeof(Dtype)));
    Dtype* bias_multiplier_data = (Dtype*)bias_multiplier_->mutable_cpu_data();
    for (int i = 0; i < M_; ++i) {
        bias_multiplier_data[i] = 1.;
    }
  }
};

template <typename Dtype>
void InnerProductLayer<Dtype>::Forward_cpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  const Dtype* bottom_data = bottom[0]->cpu_data();
  Dtype* top_data = (*top)[0]->mutable_cpu_data();
  const Dtype* weight = this->blobs_[0].cpu_data();
  caffeine_cpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, M_, N_, K_, (Dtype)1.,
      bottom_data, weight, (Dtype)0., top_data);
  if (biasterm_) {
    caffeine_cpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, M_, N_, 1, (Dtype)1.,
        (Dtype*)bias_multiplier_->cpu_data(), this->blobs_[1].cpu_data(),
        (Dtype)1., top_data);
  }
}

template <typename Dtype>
Dtype InnerProductLayer<Dtype>::Backward_cpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* top_diff = top[0]->cpu_diff();
  const Dtype* bottom_data = (*bottom)[0]->cpu_data();
  // Gradient with respect to weight
  caffeine_cpu_gemm<Dtype>(CblasTrans, CblasNoTrans, K_, N_, M_, (Dtype)1.,
      bottom_data, top_diff, (Dtype)0., this->blobs_[0].mutable_cpu_diff());
  if (biasterm_) {
    // Gradient with respect to bias
    caffeine_cpu_gemv<Dtype>(CblasTrans, M_, N_, (Dtype)1., top_diff,
        (Dtype*)bias_multiplier_->cpu_data(), (Dtype)0.,
        this->blobs_[1].mutable_cpu_diff());
  }
  if (propagate_down) {
    // Gradient with respect to bottom data
    caffeine_cpu_gemm<Dtype>(CblasNoTrans, CblasTrans, M_, K_, N_, (Dtype)1.,
        top_diff, this->blobs_[0].cpu_data(), (Dtype)0.,
        (*bottom)[0]->mutable_cpu_diff());
  }
  return Dtype(0);
}

template <typename Dtype>
void InnerProductLayer<Dtype>::Forward_gpu(const vector<Blob<Dtype>*>& bottom,
    vector<Blob<Dtype>*>* top) {
  const Dtype* bottom_data = bottom[0]->gpu_data();
  Dtype* top_data = (*top)[0]->mutable_gpu_data();
  const Dtype* weight = this->blobs_[0].gpu_data();
  caffeine_gpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, M_, N_, K_, (Dtype)1.,
      bottom_data, weight, (Dtype)0., top_data);
  if (biasterm_) {
    caffeine_gpu_gemm<Dtype>(CblasNoTrans, CblasNoTrans, M_, N_, 1, (Dtype)1.,
        (Dtype*)bias_multiplier_->gpu_data(), this->blobs_[1].gpu_data(),
        (Dtype)1., top_data);
  }
}

template <typename Dtype>
Dtype InnerProductLayer<Dtype>::Backward_gpu(const vector<Blob<Dtype>*>& top,
    const bool propagate_down,
    vector<Blob<Dtype>*>* bottom) {
  const Dtype* top_diff = top[0]->gpu_diff();
  const Dtype* bottom_data = (*bottom)[0]->gpu_data();
  // Gradient with respect to weight
  caffeine_gpu_gemm<Dtype>(CblasTrans, CblasNoTrans, K_, N_, M_, (Dtype)1.,
      bottom_data, top_diff, (Dtype)0., this->blobs_[0].mutable_gpu_diff());
  if (biasterm_) {
    // Gradient with respect to bias
    caffeine_gpu_gemv<Dtype>(CblasTrans, M_, N_, (Dtype)1., top_diff,
        (Dtype*)bias_multiplier_->gpu_data(), (Dtype)0.,
        this->blobs_[1].mutable_gpu_diff());
  }
  if (propagate_down) {
    // Gradient with respect to bottom data
    caffeine_gpu_gemm<Dtype>(CblasNoTrans, CblasTrans, M_, K_, N_, (Dtype)1.,
        top_diff, this->blobs_[0].gpu_data(), (Dtype)0.,
        (*bottom)[0]->mutable_gpu_diff());
  }
  return Dtype(0);
}

INSTANTIATE_CLASS(InnerProductLayer);

}  // namespace caffeine