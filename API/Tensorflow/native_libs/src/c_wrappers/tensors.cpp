#include "tensorflow/c/c_api.h"
#include "tensors.h"
#include "../tensor/Tensor.h"
#include "../helpers/LifeTimeManager.h"
#include "../helpers/error.h"

#include <vector>
#include <memory>
#include <random>

template<typename T> T* vector_as_array(const std::vector<T>& vec) {
    T* r = static_cast<T*>(malloc(sizeof(T) * vec.size()));
    memcpy(r, vec.data(), sizeof(T) * vec.size());
    return r;
}

TFL_API Tensor *make_tensor(void const *array, TF_DataType type, const int64_t *dims, size_t num_dims, const char **outError) {
    return TRANSLATE_EXCEPTION(outError) {
        FFILOG(array, type, dims, num_dims);
        auto tensor_ptr = std::make_shared<Tensor>(array, dims, num_dims, type);
        auto t = LifetimeManager::instance().addOwnership(std::move(tensor_ptr));
        FFILOGANDRETURN(t, array, type, dims, num_dims);
    };
}

TFL_API int get_tensor_num_dims(Tensor *tensor, const char **outError) {
    return TRANSLATE_EXCEPTION(outError) {
        auto r = LifetimeManager::instance().accessOwned(tensor)->shape().size();
        FFILOGANDRETURN(static_cast<int>(r), tensor);
    };
}

TFL_API int64_t get_tensor_dim(Tensor *tensor, int32_t dim_index, const char **outError) {
    return TRANSLATE_EXCEPTION(outError) {
        auto r = LifetimeManager::instance().accessOwned(tensor)->shape()[dim_index];
        FFILOGANDRETURN(r, tensor, dim_index);
    };
}

TFL_API int64_t* get_tensor_dims(Tensor *tensor, const char **outError) {
    return TRANSLATE_EXCEPTION(outError) {
        auto shape = LifetimeManager::instance().accessOwned(tensor)->shape();
        auto r = vector_as_array(shape);
        FFILOGANDRETURN(r, tensor);
    };
}

TFL_API int64_t get_tensor_flatlist_length(Tensor* tensor, const char **outError) {
    return TRANSLATE_EXCEPTION(outError) {
        int64_t l = LifetimeManager::instance().accessOwned(tensor)->flatSize();
        FFILOGANDRETURN(l, tensor);
    };
}

#define GET_TENSOR_VALUE_AT(typelabel) \
TFL_API Type<typelabel>::lunatype get_tensor_value_at_##typelabel(Tensor *tensor, int64_t *idxs, size_t len, const char **outError) { \
    return TRANSLATE_EXCEPTION(outError) { \
        auto r = LifetimeManager::instance().accessOwned(tensor)->at<typelabel>(idxs, len); \
        FFILOGANDRETURN(r, tensor, idxs, len); \
    }; \
}

#define GET_TENSOR_VALUE_AT_INDEX(typelabel) \
TFL_API Type<typelabel>::lunatype get_tensor_value_at_index_##typelabel(Tensor *tensor, int64_t index, const char **outError) { \
    return TRANSLATE_EXCEPTION(outError) { \
        auto r = LifetimeManager::instance().accessOwned(tensor)->at<typelabel>(index); \
        FFILOGANDRETURN(r, tensor, index); \
    }; \
}

#define TENSOR_TO_FLATLIST(typelabel) \
TFL_API Type<typelabel>::lunatype* tensor_to_flatlist_##typelabel(Tensor* tensor, const char **outError) { \
    return TRANSLATE_EXCEPTION(outError) { \
        auto t = LifetimeManager::instance().accessOwned(tensor); \
        auto len = t->flatSize(); \
        Type<typelabel>::lunatype* r = static_cast<Type<typelabel>::lunatype*> (malloc(sizeof(Type<typelabel>::lunatype) * len)); \
        for (size_t i = 0; i < len; ++i) { \
            r[i] = t->at<typelabel>(i); \
        } \
        FFILOGANDRETURN(r, tensor); \
    }; \
}

#define MAKE_RANDOM_TENSOR(typelabel, type) \
TFL_API Tensor *make_random_tensor_##typelabel(const int64_t *dims, size_t num_dims, \
    Type<typelabel>::lunatype const min, Type<typelabel>::lunatype const max, const char **outError){ \
    return TRANSLATE_EXCEPTION(outError) { \
        int64_t elems = std::accumulate(dims, dims+num_dims, 1, std::multiplies<int64_t>()); \
        auto* data = (Type<typelabel>::tftype*) malloc(elems * TF_DataTypeSize(typelabel)); \
        \
        std::mt19937 engine(std::random_device{}()); \
        std::uniform_##type##_distribution distribution(min, max); \
        \
        std::generate(data, data+elems, [&]() { \
                return distribution(engine); \
        }); \
        \
        auto tensor = std::make_shared<Tensor>(data, dims, num_dims, typelabel); \
        free(data); \
        \
        return LifetimeManager::instance().addOwnership(std::move(tensor));\
    }; \
}

#define MAKE_CONST_TENSOR(typelabel, type) \
TFL_API Tensor *make_const_tensor_##typelabel(const int64_t *dims, size_t num_dims, \
	Type<typelabel>::lunatype const value, const char **outError){ \
	return TRANSLATE_EXCEPTION(outError) { \
        int64_t elems = std::accumulate(dims, dims+num_dims, 1, std::multiplies<int64_t>()); \
        auto* data = (Type<typelabel>::tftype*) malloc(elems * TF_DataTypeSize(typelabel)); \
        \
        std::generate(data, data+elems, [&]() { \
                return value; \
        }); \
        \
        auto tensor = std::make_shared<Tensor>(data, dims, num_dims, typelabel); \
        free(data); \
        \
        return LifetimeManager::instance().addOwnership(std::move(tensor));\
    }; \
}



#define DEFINE_TENSOR(typelabel) \
GET_TENSOR_VALUE_AT(typelabel); \
GET_TENSOR_VALUE_AT_INDEX(typelabel); \
TENSOR_TO_FLATLIST(typelabel);

#define DEFINE_TENSOR_NUMERIC(typelabel, randomtype) \
DEFINE_TENSOR(typelabel); \
MAKE_RANDOM_TENSOR(typelabel, randomtype);\
MAKE_CONST_TENSOR(typelabel, randomtype);\


DEFINE_TENSOR_NUMERIC(TF_FLOAT, real);
DEFINE_TENSOR_NUMERIC(TF_DOUBLE, real);
DEFINE_TENSOR_NUMERIC(TF_INT8, int);
DEFINE_TENSOR_NUMERIC(TF_INT16, int);
DEFINE_TENSOR_NUMERIC(TF_INT32, int);
DEFINE_TENSOR_NUMERIC(TF_INT64, int);
DEFINE_TENSOR_NUMERIC(TF_UINT8, int);
DEFINE_TENSOR_NUMERIC(TF_UINT16, int);
DEFINE_TENSOR_NUMERIC(TF_UINT32, int);
DEFINE_TENSOR_NUMERIC(TF_UINT64, int);
DEFINE_TENSOR(TF_BOOL);
DEFINE_TENSOR(TF_STRING);
//DEFINE_TENSOR(TF_HALF);
