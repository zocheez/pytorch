#ifndef THNN_OMP_OVERHEAD_THRESHOLD
#define THNN_OMP_OVERHEAD_THRESHOLD 5000
#endif
#ifndef TH_GENERIC_FILE
#define TH_GENERIC_FILE "generic/HardTanh.c"
#else

void THNN_(HardTanh_updateOutput)(
          THNNState *state,
          THTensor *input,
          THTensor *output,
          accreal min_val_,
          accreal max_val_,
          bool inplace)
{
  real min_val = TH_CONVERT_ACCREAL_TO_REAL(min_val_);
  real max_val = TH_CONVERT_ACCREAL_TO_REAL(max_val_);
  if (inplace)
    THTensor_(set)(output, input);
  else
    THTensor_(resizeAs)(output, input);

  if (input->nDimension == 1 || !THTensor_(isContiguous)(input) || !THTensor_(isContiguous)(output))
  {
    if (inplace)
    {
      TH_TENSOR_APPLY(real, input,
        if (*input_data < min_val)
          *input_data = min_val;
        else if (*input_data > max_val)
          *input_data = max_val;
      );
    }
    else
    {
      int serial_path = 0;
#ifdef _OPENMP
      int inOMP = omp_in_parallel();
      if (inOMP) {
        serial_path = 1;
      } else {
        int64_t output_size = THTensor_(nElement)(output);
        int output_contig = THTensor_(isContiguous)(output);
        int input_contig = THTensor_(isContiguous)(input);
        TH_TENSOR_APPLY2_OMP(output_size, output_contig, input_contig,
          real, output, real, input,
          if (*input_data < min_val)
            *output_data = min_val;
          else if (*input_data <= max_val)
            *output_data = *input_data;
          else
            *output_data = max_val;,
          THNN_OMP_OVERHEAD_THRESHOLD
        );
      }
#else
      serial_path = 1;
#endif
      if (serial_path) {
        TH_TENSOR_APPLY2(real, output, real, input,
          if (*input_data < min_val)
            *output_data = min_val;
          else if (*input_data <= max_val)
            *output_data = *input_data;
          else
            *output_data = max_val;
        );
      }
    }
  }
  else
  {
    real* ptr_input  = THTensor_(data)(input);
    real* ptr_output = THTensor_(data)(output);
    ptrdiff_t i;
    ptrdiff_t n = THTensor_(nElement)(input);

    if (inplace)
#pragma omp parallel for private(i)
      for (i = 0; i < n; i++)
      {
        if (ptr_input[i] < min_val)
          ptr_input[i] = min_val;
        else if (ptr_input[i] > max_val)
          ptr_input[i] = max_val;
      }
    else
#pragma omp parallel for private(i)
      for (i = 0; i < n; i++)
      {
        if (ptr_input[i] < min_val)
          ptr_output[i] = min_val;
        else if (ptr_input[i] <= max_val)
          ptr_output[i] = ptr_input[i];
        else
          ptr_output[i] = max_val;
      }
  }
}

void THNN_(HardTanh_updateGradInput)(
          THNNState *state,
          THTensor *input,
          THTensor *gradOutput,
          THTensor *gradInput,
          accreal min_val_,
          accreal max_val_,
          bool inplace)
{
  real min_val = TH_CONVERT_ACCREAL_TO_REAL(min_val_);
  real max_val = TH_CONVERT_ACCREAL_TO_REAL(max_val_);

  THNN_CHECK_NELEMENT(input, gradOutput);
  if (inplace)
    THTensor_(set)(gradInput, gradOutput);
  else
    THTensor_(resizeAs)(gradInput, input);

  if (input->nDimension == 1 ||
    !THTensor_(isContiguous)(input) ||
    !THTensor_(isContiguous)(gradOutput) ||
    !THTensor_(isContiguous)(gradInput))
  {
    if (inplace)
    {
      int serial_path = 0;
#ifdef _OPENMP
      int inOMP = omp_in_parallel();
      if (inOMP) {
        serial_path = 1;
      } else {
        int64_t gradOutput_size = THTensor_(nElement)(gradOutput);
        int gradOutput_contig = THTensor_(isContiguous)(gradOutput);
        int input_contig = THTensor_(isContiguous)(input);
        TH_TENSOR_APPLY2_OMP(gradOutput_size, gradOutput_contig, input_contig,
          real, gradOutput, real, input,
          if (*input_data <= min_val || *input_data >= max_val)
            *gradOutput_data = 0;,
          THNN_OMP_OVERHEAD_THRESHOLD
        );
      }
#else
      serial_path = 1;
#endif
      if (serial_path) {
        TH_TENSOR_APPLY2(real, gradOutput, real, input,
          if (*input_data <= min_val || *input_data >= max_val)
            *gradOutput_data = 0;
        );
      }
    }
    else {
      int serial_path = 0;
#ifdef _OPENMP
      int inOMP = omp_in_parallel();
      if (inOMP) {
        serial_path = 1;
      } else {
        int64_t gradInput_size = THTensor_(nElement)(gradInput);
        int gradInput_contig = THTensor_(isContiguous)(gradInput);
        int gradOutput_contig = THTensor_(isContiguous)(gradOutput);
        int input_contig = THTensor_(isContiguous)(input);
        TH_TENSOR_APPLY3_OMP(gradInput_size, gradInput_contig, gradOutput_contig, input_contig,
          real, gradInput, real, gradOutput, real, input,
          if (*input_data <= min_val || *input_data >= max_val)
            *gradInput_data = 0;
          else
            *gradInput_data = *gradOutput_data;,
          THNN_OMP_OVERHEAD_THRESHOLD
        );
      }
#else
      serial_path = 1;
#endif
      if (serial_path) {
        TH_TENSOR_APPLY3(real, gradInput, real, gradOutput, real, input,
          if (*input_data <= min_val || *input_data >= max_val)
            *gradInput_data = 0;
          else
            *gradInput_data = *gradOutput_data;
        );
      }
    }
  }
  else
  {
    real* ptr_gradOutput = THTensor_(data)(gradOutput);
    real* ptr_gradInput  = THTensor_(data)(gradInput);
    real* ptr_input      = THTensor_(data)(input);
    ptrdiff_t i;
    ptrdiff_t n = THTensor_(nElement)(input);

    if (inplace)
#pragma omp parallel for private(i)
      for (i = 0; i < n; i++)
      {
        if (ptr_input[i] <= min_val || ptr_input[i] >= max_val)
          ptr_gradInput[i] = 0;
      }
    else
#pragma omp parallel for private(i)
      for (i = 0; i < n; i++)
      {
        if (ptr_input[i] <= min_val || ptr_input[i] >= max_val)
          ptr_gradInput[i] = 0;
        else
          ptr_gradInput[i] = ptr_gradOutput[i];
      }
  }
}

#undef THNN_OMP_OVERHEAD_THRESHOLD
#endif
