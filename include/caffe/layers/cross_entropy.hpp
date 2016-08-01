#ifndef CAFFE_CROSS_ENTROPY_LAYER_HPP
#define CAFFE_CROSS_ENTROPY_LAYER_HPP

#include <vector>

#include "caffe/blob.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"

namespace caffe {

  template <typename Dtype>
  class CrossEntropyLayer : public Layer<Dtype> {
  public:
    explicit CrossEntropyLayer(const LayerParameter& param)
      : Layer<Dtype>(param) {}
    virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
			 const vector<Blob<Dtype>*>& top);

    virtual inline const char* type() const { return "CrossEntropy"; }
    virtual inline int ExactNumBottomBlobs() const { return 2; }
    virtual inline int ExactNumTopBlobs() const { return 1; }

  protected:
    virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
			     const vector<Blob<Dtype>*>& top);
    virtual void Backward_cpu(const vector<Blob<Dtype>*>& top,
			      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom);
    virtual void Backward_gpu(const vector<Blob<Dtype>*>& top,
			      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom);

    int outer_num_;
    int inner_num_;
  };
  
}

#endif
