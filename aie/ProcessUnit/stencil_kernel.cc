#include "stencil_kernel.h"

void jacobi_2d3_f32(
    adf::input_buffer<float,adf::extents<3*IN_ROW>>& __restrict in0,
    adf::output_buffer<float,adf::extents<ROW>>& __restrict out0
){
    
    const float* ptop = in0.data();
    const float* pmid = ptop + IN_ROW;
    const float* pbot = pmid + IN_ROW;
    float* pout = out0.data();

    aie::vector<float,w> k_center = aie::broadcast<float,w>(1.0f);
    aie::vector<float,w> k_neighbor = aie::broadcast<float,w>(1.0f);

    for (int i = 0; i<ROW; i += w){
        auto top = aie::load_unaligned_v<w>(ptop+ i +1);
        auto mid = aie::load_unaligned_v<w>(pmid+ i +1);
        auto bot = aie::load_unaligned_v<w>(pbot+ i +1);
        auto left = aie::load_unaligned_v<w>(pmid + i);
        auto right = aie::load_unaligned_v<w>(pmid + i + 2);

        aie::accum<accfloat,w> acc = aie::zeros<accfloat,w>();
        acc = aie::mac(acc, mid,   k_center);
        acc = aie::mac(acc, top,   k_neighbor);
        acc = aie::mac(acc, bot,   k_neighbor);
        acc = aie::mac(acc, left,  k_neighbor);
        acc = aie::mac(acc, right, k_neighbor);

        aie::store_v(pout + i,aie::to_vector<float>(acc));

        #if defined(__AIESIM__) || defined(__X86SIM__)
                aie::print(aie::to_vector<float>(acc), true, "out: ");
        #endif

    }
}