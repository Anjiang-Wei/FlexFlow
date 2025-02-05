# The basis for this test of the 'add' operation is generated by ChatGPT using the manually created conv2d.py as a template.


import flexflow.core
import numpy as np
from flexflow.core import *

def test_add(ffconfig, input_arr1: np.ndarray, input_arr2: np.ndarray) -> flexflow.core.Tensor:
    ffmodel = FFModel(ffconfig)

    input_tensor1 = ffmodel.create_tensor(input_arr1.shape, DataType.DT_FLOAT)
    input_tensor2 = ffmodel.create_tensor(input_arr2.shape, DataType.DT_FLOAT)

    out = ffmodel.add(input_tensor1, input_tensor2)

    ffoptimizer = SGDOptimizer(ffmodel, 0.001)
    ffmodel.optimizer = ffoptimizer
    ffmodel.compile(
        loss_type=LossType.LOSS_SPARSE_CATEGORICAL_CROSSENTROPY,
        metrics=[MetricsType.METRICS_ACCURACY, MetricsType.METRICS_SPARSE_CATEGORICAL_CROSSENTROPY])
    dataloader_input1 = ffmodel.create_data_loader(input_tensor1, input_arr1)
    dataloader_input2 = ffmodel.create_data_loader(input_tensor2, input_arr2)

    ffmodel.init_layers()

    dataloader_input1.reset()
    dataloader_input1.next_batch(ffmodel)

    dataloader_input2.reset()
    dataloader_input2.next_batch(ffmodel)

    ffmodel.forward()

    out.inline_map(ffmodel, ffconfig)
    return out.get_array(ffmodel, ffconfig)


if __name__ == '__main__':
    init_flexflow_runtime()
    ffconfig = FFConfig()

    input1 = np.random.randn(ffconfig.batch_size, 5, 10, 10).astype(np.float32)
    input2 = np.random.randn(ffconfig.batch_size, 5, 10, 10).astype(np.float32)

    _ = test_add(ffconfig, input1, input2)
