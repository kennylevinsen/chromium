# Copyright 2019 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from .code_generator_info import CodeGeneratorInfo
from .composition_parts import WithCodeGeneratorInfo
from .composition_parts import WithComponent
from .composition_parts import WithDebugInfo
from .composition_parts import WithExtendedAttributes
from .constant import Constant
from .operation import Operation
from .operation import OperationGroup

from .ir_map import IRMap
from .make_copy import make_copy
from .user_defined_type import UserDefinedType


class CallbackInterface(UserDefinedType, WithExtendedAttributes,
                        WithCodeGeneratorInfo, WithComponent, WithDebugInfo):
    """https://heycam.github.io/webidl/#idl-interfaces"""

    class IR(IRMap.IR, WithExtendedAttributes, WithCodeGeneratorInfo,
             WithComponent, WithDebugInfo):
        def __init__(self,
                     identifier,
                     constants=None,
                     operations=None,
                     extended_attributes=None,
                     component=None,
                     debug_info=None):
            if constants is None:
                constants = []
            if operations is None:
                operations = []
            assert isinstance(constants, (list, tuple))
            assert isinstance(operations, (list, tuple))
            assert all(
                isinstance(constant, Constant.IR) for constant in constants)
            assert all(
                isinstance(operation, Operation.IR)
                for operation in operations)

            IRMap.IR.__init__(
                self,
                identifier=identifier,
                kind=IRMap.IR.Kind.CALLBACK_INTERFACE)
            WithExtendedAttributes.__init__(self, extended_attributes)
            WithCodeGeneratorInfo.__init__(self)
            WithComponent.__init__(self, component)
            WithDebugInfo.__init__(self, debug_info)

            self.attributes = []
            self.constants = constants
            self.operations = operations
            self.operation_groups = []
            self.constructors = []
            self.constructor_groups = []

    def __init__(self, ir):
        assert isinstance(ir, CallbackInterface.IR)

        ir = make_copy(ir)
        UserDefinedType.__init__(self, ir.identifier)
        WithExtendedAttributes.__init__(self, ir.extended_attributes)
        WithCodeGeneratorInfo.__init__(
            self, CodeGeneratorInfo(ir.code_generator_info))
        WithComponent.__init__(self, components=ir.components)
        WithDebugInfo.__init__(self, ir.debug_info)
        self._constants = tuple([
            Constant(constant_ir, owner=self) for constant_ir in ir.constants
        ])
        self._operations = tuple([
            Operation(operation_ir, owner=self)
            for operation_ir in ir.operations
        ])
        self._operation_groups = tuple([
            OperationGroup(
                operation_group_ir,
                filter(lambda x: x.identifier == operation_group_ir.identifier,
                       self._operations),
                owner=self) for operation_group_ir in ir.operation_groups
        ])

    @property
    def constants(self):
        """Returns constants."""
        return self._constants

    @property
    def operations(self):
        """Returns operations."""
        return self._operations

    @property
    def operation_groups(self):
        """Returns groups of overloaded operations."""
        return self._operation_groups

    # UserDefinedType overrides
    @property
    def is_callback_interface(self):
        return True
