cmake_minimum_required(VERSION 3.12.0)

if(WIN32)
  link_libraries(iphlpapi)
endif()

project(ACE_for_OpenDDS CXX)
option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

set(ACE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(HEADER_FILES_ACE_FOR_OPENDDS ACE.h ACE_export.h ARGV.h Abstract_Timer_Queue.h Acceptor.h Active_Map_Manager_T.h Addr.h Arg_Shifter.h Argv_Type_Converter.h Array_Base.h Array_Map.h Assert.h Atomic_Op.h Atomic_Op_GCC_T.h Atomic_Op_Sparc.h Atomic_Op_T.h Auto_Event.h Auto_Functor.h Auto_IncDec_T.h Auto_Ptr.h Base_Thread_Adapter.h Based_Pointer_Repository.h Based_Pointer_T.h Basic_Stats.h Basic_Types.h Bound_Ptr.h CDR_Base.h CDR_Size.h CDR_Stream.h CORBA_macros.h Cache_Map_Manager_T.h Cached_Connect_Strategy_T.h Caching_Strategies_T.h Caching_Utility_T.h Cleanup.h Cleanup_Strategies_T.h Codeset_Symbols.h Condition_Attributes.h Condition_Recursive_Thread_Mutex.h Condition_T.h Condition_Thread_Mutex.h Configuration.h Configuration_Import_Export.h Connector.h Containers.h Containers_T.h Copy_Disabled.h Countdown_Time.h Countdown_Time_T.h DLL.h DLL_Manager.h Default_Constants.h Dev_Poll_Reactor.h Dirent.h Dump_T.h Dynamic_Message_Strategy.h Dynamic_Service.h Dynamic_Service_Base.h Dynamic_Service_Dependency.h Encoding_Converter.h Encoding_Converter_Factory.h Env_Value_T.h Event.h Event_Base.h Event_Handler.h Event_Handler_Handle_Timeout_Upcall.h Event_Handler_T.h Flag_Manip.h Framework_Component.h Framework_Component_T.h Free_List.h Functor.h Functor_String.h Functor_T.h Get_Opt.h Global_Macros.h Guard_T.h Handle_Gobbler.h Handle_Set.h Hash_Cache_Map_Manager_T.h Hash_Map_Manager.h Hash_Map_Manager_T.h Hash_Map_With_Allocator_T.h Hashable.h High_Res_Timer.h INET_Addr.h IOStream_T.h IO_Cntl_Msg.h IPC_SAP.h If_Then_Else.h Init_ACE.h Intrusive_Auto_Ptr.h Intrusive_List.h Intrusive_List_Node.h Lib_Find.h Local_Memory_Pool.h Lock.h Lock_Adapter_T.h Log_Category.h Log_Msg.h Log_Msg_Backend.h Log_Msg_Callback.h Log_Msg_IPC.h Log_Msg_NT_Event_Log.h Log_Msg_UNIX_Syslog.h Log_Priority.h Log_Record.h Logging_Strategy.h MMAP_Memory_Pool.h Malloc.h Malloc_Allocator.h Malloc_Base.h Malloc_T.h Managed_Object.h Manual_Event.h Map_Manager.h Map_T.h Mem_Map.h Memory_Pool.h Message_Block.h Message_Block_T.h Message_Queue.h Message_Queue_NT.h Message_Queue_T.h Message_Queue_Vx.h Metrics_Cache.h Min_Max.h Module.h Monotonic_Time_Policy.h Mutex.h Node.h Notification_Queue.h Notification_Strategy.h Null_Barrier.h Null_Condition.h Null_Mutex.h Null_Semaphore.h Numeric_Limits.h OS.h OS_Errno.h OS_Log_Msg_Attributes.h OS_Memory.h OS_NS_Thread.h OS_NS_arpa_inet.h OS_NS_ctype.h OS_NS_dirent.h OS_NS_dlfcn.h OS_NS_errno.h OS_NS_fcntl.h OS_NS_macros.h OS_NS_math.h OS_NS_netdb.h OS_NS_poll.h OS_NS_pwd.h OS_NS_regex.h OS_NS_signal.h OS_NS_stdio.h OS_NS_stdlib.h OS_NS_string.h OS_NS_strings.h OS_NS_stropts.h OS_NS_sys_mman.h OS_NS_sys_msg.h OS_NS_sys_resource.h OS_NS_sys_select.h OS_NS_sys_sendfile.h OS_NS_sys_shm.h OS_NS_sys_socket.h OS_NS_sys_stat.h OS_NS_sys_time.h OS_NS_sys_uio.h OS_NS_sys_utsname.h OS_NS_sys_wait.h OS_NS_time.h OS_NS_unistd.h OS_NS_wchar.h OS_NS_wctype.h OS_QoS.h OS_Thread_Adapter.h OS_main.h Obchunk.h Object_Manager.h Object_Manager_Base.h Obstack.h Obstack_T.h Pair_T.h Parse_Node.h Pipe.h Process.h RB_Tree.h RW_Mutex.h RW_Thread_Mutex.h Reactor.h Reactor_Impl.h Reactor_Timer_Interface.h Reactor_Token_T.h Recursive_Thread_Mutex.h Refcountable_T.h Refcounted_Auto_Ptr.h Reverse_Lock_T.h SOCK.h SOCK_Acceptor.h SOCK_Connector.h SOCK_Dgram.h SOCK_Dgram_Mcast.h SOCK_IO.h SOCK_Stream.h SString.h SStringfwd.h Sched_Params.h Select_Reactor.h Select_Reactor_Base.h Select_Reactor_T.h Service_Config.h Service_Gestalt.h Service_Manager.h Service_Object.h Service_Repository.h Service_Types.h Shared_Object.h Sig_Adapter.h Sig_Handler.h Signal.h Singleton.h Sock_Connect.h Stack_Trace.h Static_Object_Lock.h Stats.h Strategies_T.h Stream.h Stream_Modules.h String_Base.h String_Base_Const.h Svc_Conf.h Svc_Conf_Lexer.h Svc_Conf_Param.h Svc_Conf_Token_Table.h Svc_Conf_Tokens.h Svc_Handler.h Synch.h Synch_Traits.h TP_Reactor.h TSS_Adapter.h TSS_T.h Task.h Task_Ex_T.h Task_T.h Thread.h Thread_Adapter.h Thread_Control.h Thread_Exit.h Thread_Hook.h Thread_Manager.h Thread_Mutex.h Time_Policy.h Time_Policy_T.h Time_Value.h Time_Value_T.h Timeprobe_T.h Timer_Hash.h Timer_Hash_T.h Timer_Heap.h Timer_Heap_T.h Timer_List.h Timer_List_T.h Timer_Queue.h Timer_Queue_Adapters.h Timer_Queue_Iterator.h Timer_Queue_T.h Timer_Queuefwd.h Timer_Wheel.h Timer_Wheel_T.h Token.h Tokenizer_T.h Trace.h Truncate.h Typed_SV_Message.h Typed_SV_Message_Queue.h UPIPE_Addr.h UTF16_Encoding_Converter.h UTF32_Encoding_Converter.h UTF8_Encoding_Converter.h Unbounded_Queue.h Unbounded_Set.h Unbounded_Set_Ex.h Value_Ptr.h Vector_T.h Version.h Versioned_Namespace.h XML_Svc_Conf.h ace_wchar.h checked_iterator.h config-WinCE.h config-aix-5.x.h config-aix-7.h config-all.h config-android.h config-cygwin32.h config-face-conftest.h config-face-safety.h config-freebsd.h config-g++-common.h config-hpux-11.00.h config-hurd.h config-icc-common.h config-integritySCA.h config-kfreebsd.h config-linux-common.h config-linux.h config-lite.h config-lynxos-178.h config-lynxos.h config-macosx-elcapitan.h config-macosx-highsierra.h config-macosx-iOS.h config-macosx-jaguar.h config-macosx-leopard.h config-macosx-lion.h config-macosx-mavericks.h config-macosx-mojave.h config-macosx-mountainlion.h config-macosx-panther.h config-macosx-sierra.h config-macosx-snowleopard.h config-macosx-tiger.h config-macosx-yosemite.h config-macosx.h config-macros.h config-mqx.h config-netbsd.h config-openbsd.h config-openvms.h config-pharlap.h config-posix-nonetworking.h config-posix.h config-qnx.h config-rtems.h config-suncc-common.h config-sunos5.10.h config-sunos5.11.h config-sunos5.4-g++.h config-sunos5.4-sunc++-4.x.h config-sunos5.5.h config-sunos5.6.h config-sunos5.7.h config-sunos5.8.h config-sunos5.9.h config-vxworks.h config-vxworks21.03.h config-vxworks22.03.h config-vxworks6.4.h config-vxworks6.5.h config-vxworks6.6.h config-vxworks6.7.h config-vxworks6.8.h config-vxworks6.9.h config-vxworks7.0.h config-win32-borland.h config-win32-common.h config-win32-mingw.h config-win32-mingw64.h config-win32-msvc-14.h config-win32-msvc-141.h config-win32-msvc-142.h config-win32-msvc-143.h config-win32-msvc.h config-win32.h config-windows.h config.h iosfwd.h post.h pre.h streams.h svc_export.h)
set(HEADER_FILES_ACE_FOR_OPENDDS_ARPA  os_include/arpa/os_inet.h)
set(HEADER_FILES_ACE_FOR_OPENDDS_NET os_include/net/os_if.h)
set(HEADER_FILES_ACE_FOR_OPENDDS_NETINET os_include/netinet/os_in.h os_include/netinet/os_tcp.h)
set(HEADER_FILES_ACE_FOR_OPENDDS_OS os_include/os_aio.h os_include/os_assert.h os_include/os_byteswap.h os_include/os_complex.h os_include/os_cpio.h os_include/os_ctype.h os_include/os_dirent.h os_include/os_dlfcn.h os_include/os_errno.h os_include/os_fcntl.h os_include/os_fenv.h os_include/os_float.h os_include/os_fmtmsg.h os_include/os_fnmatch.h os_include/os_ftw.h os_include/os_glob.h os_include/os_grp.h os_include/os_iconv.h os_include/os_ifaddrs.h os_include/os_intrin.h os_include/os_inttypes.h os_include/os_iso646.h os_include/os_kstat.h os_include/os_langinfo.h os_include/os_libgen.h os_include/os_limits.h os_include/os_local.h os_include/os_math.h os_include/os_monetary.h os_include/os_mqueue.h os_include/os_ndbm.h os_include/os_netdb.h os_include/os_nl_types.h os_include/os_pdh.h os_include/os_pdhmsg.h os_include/os_poll.h os_include/os_pthread.h os_include/os_pwd.h os_include/os_regex.h os_include/os_sched.h os_include/os_search.h os_include/os_semaphore.h os_include/os_setjmp.h os_include/os_signal.h os_include/os_spawn.h os_include/os_stdarg.h os_include/os_stdbool.h os_include/os_stddef.h os_include/os_stdint.h os_include/os_stdio.h os_include/os_stdlib.h os_include/os_string.h os_include/os_strings.h os_include/os_stropts.h os_include/os_syslog.h os_include/os_tar.h os_include/os_termios.h os_include/os_tgmath.h os_include/os_time.h os_include/os_trace.h os_include/os_typeinfo.h os_include/os_ucontext.h os_include/os_ulimit.h os_include/os_unistd.h os_include/os_utime.h os_include/os_utmpx.h os_include/os_wchar.h os_include/os_wctype.h os_include/os_wordexp.h)
set(HEADER_FILES_ACE_FOR_OPENDDS_SYS os_include/sys/os_ipc.h os_include/sys/os_loadavg.h os_include/sys/os_mman.h os_include/sys/os_msg.h os_include/sys/os_pstat.h os_include/sys/os_resource.h os_include/sys/os_select.h os_include/sys/os_sem.h os_include/sys/os_shm.h os_include/sys/os_socket.h os_include/sys/os_stat.h os_include/sys/os_statvfs.h os_include/sys/os_sysctl.h os_include/sys/os_sysinfo.h os_include/sys/os_time.h os_include/sys/os_timeb.h os_include/sys/os_times.h os_include/sys/os_types.h os_include/sys/os_uio.h os_include/sys/os_un.h os_include/sys/os_utsname.h os_include/sys/os_wait.h)

set(INLINE_FILES_ACE_FOR_OPENDDS ACE.inl ARGV.inl Active_Map_Manager_T.inl Addr.inl Argv_Type_Converter.inl Array_Base.inl Array_Map.inl Atomic_Op.inl Atomic_Op_GCC_T.inl Atomic_Op_T.inl Auto_Functor.inl Auto_IncDec_T.inl Auto_Ptr.inl Base_Thread_Adapter.inl Based_Pointer_T.inl Basic_Stats.inl Bound_Ptr.inl CDR_Base.inl CDR_Size.inl CDR_Stream.inl Cache_Map_Manager_T.inl Caching_Strategies_T.inl Cleanup.inl Condition_Attributes.inl Condition_T.inl Condition_Thread_Mutex.inl Configuration.inl Containers.inl Containers_T.inl Countdown_Time_T.inl Dev_Poll_Reactor.inl Dirent.inl Dynamic_Message_Strategy.inl Dynamic_Service.inl Env_Value_T.inl Event.inl Event_Base.inl Event_Handler_Handle_Timeout_Upcall.inl Event_Handler_T.inl Flag_Manip.inl Framework_Component.inl Functor.inl Functor_String.inl Functor_T.inl Get_Opt.inl Guard_T.inl Handle_Gobbler.inl Handle_Set.inl Hash_Cache_Map_Manager_T.inl Hash_Map_Manager_T.inl Hash_Map_With_Allocator_T.inl High_Res_Timer.inl INET_Addr.inl IOStream_T.inl IO_Cntl_Msg.inl IPC_SAP.inl Intrusive_Auto_Ptr.inl Intrusive_List.inl Intrusive_List_Node.inl Lock.inl Lock_Adapter_T.inl Log_Category.inl Log_Msg.inl Log_Record.inl MMAP_Memory_Pool.inl Malloc.inl Malloc_Allocator.inl Malloc_T.inl Managed_Object.inl Map_Manager.inl Map_T.inl Mem_Map.inl Message_Block.inl Message_Block_T.inl Message_Queue.inl Message_Queue_NT.inl Message_Queue_Vx.inl Module.inl Monotonic_Time_Policy.inl Mutex.inl Notification_Queue.inl Notification_Strategy.inl OS_Errno.inl OS_Log_Msg_Attributes.inl OS_NS_Thread.inl OS_NS_arpa_inet.inl OS_NS_ctype.inl OS_NS_dirent.inl OS_NS_dlfcn.inl OS_NS_errno.inl OS_NS_fcntl.inl OS_NS_math.inl OS_NS_netdb.inl OS_NS_poll.inl OS_NS_pwd.inl OS_NS_regex.inl OS_NS_signal.inl OS_NS_stdio.inl OS_NS_stdlib.inl OS_NS_string.inl OS_NS_strings.inl OS_NS_stropts.inl OS_NS_sys_mman.inl OS_NS_sys_msg.inl OS_NS_sys_resource.inl OS_NS_sys_select.inl OS_NS_sys_sendfile.inl OS_NS_sys_shm.inl OS_NS_sys_socket.inl OS_NS_sys_stat.inl OS_NS_sys_time.inl OS_NS_sys_uio.inl OS_NS_sys_wait.inl OS_NS_time.inl OS_NS_unistd.inl OS_NS_wchar.inl OS_NS_wctype.inl Object_Manager.inl Obstack_T.inl Pair_T.inl Pipe.inl Process.inl RB_Tree.inl RW_Mutex.inl RW_Thread_Mutex.inl Reactor.inl Recursive_Thread_Mutex.inl Refcountable_T.inl Refcounted_Auto_Ptr.inl Reverse_Lock_T.inl SOCK.inl SOCK_Acceptor.inl SOCK_Connector.inl SOCK_Dgram.inl SOCK_Dgram_Mcast.inl SOCK_IO.inl SOCK_Stream.inl SString.inl Sched_Params.inl Select_Reactor_Base.inl Select_Reactor_T.inl Service_Config.inl Service_Gestalt.inl Service_Object.inl Service_Repository.inl Service_Types.inl Shared_Object.inl Sig_Handler.inl Signal.inl Singleton.inl Stats.inl Strategies_T.inl Stream.inl String_Base.inl TP_Reactor.inl TSS_T.inl Task.inl Task_Ex_T.inl Task_T.inl Thread.inl Thread_Adapter.inl Thread_Control.inl Thread_Manager.inl Thread_Mutex.inl Time_Policy.inl Time_Policy_T.inl Time_Value.inl Time_Value_T.inl Timer_Queue_Adapters.inl Timer_Queue_Iterator.inl Timer_Queue_T.inl Token.inl Typed_SV_Message.inl Typed_SV_Message_Queue.inl UTF16_Encoding_Converter.inl Unbounded_Queue.inl Unbounded_Set.inl Unbounded_Set_Ex.inl Vector_T.inl ace_wchar.inl)

set(TEMPLATE_FILES_ACE_FOR_OPENDDS ARGV.cpp Abstract_Timer_Queue.cpp Acceptor.cpp Active_Map_Manager_T.cpp Arg_Shifter.cpp Array_Base.cpp Array_Map.cpp Atomic_Op_GCC_T.cpp Atomic_Op_T.cpp Auto_Event.cpp Auto_Functor.cpp Auto_IncDec_T.cpp Auto_Ptr.cpp Based_Pointer_T.cpp Bound_Ptr.cpp Cache_Map_Manager_T.cpp Cached_Connect_Strategy_T.cpp Caching_Strategies_T.cpp Caching_Utility_T.cpp Cleanup_Strategies_T.cpp Condition_T.cpp Connector.cpp Containers_T.cpp Countdown_Time_T.cpp Dump_T.cpp Dynamic_Service.cpp Env_Value_T.cpp Event.cpp Event_Handler_T.cpp Framework_Component_T.cpp Free_List.cpp Functor_T.cpp Guard_T.cpp Hash_Cache_Map_Manager_T.cpp Hash_Map_Manager_T.cpp Hash_Map_With_Allocator_T.cpp IOStream_T.cpp Intrusive_Auto_Ptr.cpp Intrusive_List.cpp Intrusive_List_Node.cpp Lock_Adapter_T.cpp Malloc_T.cpp Managed_Object.cpp Manual_Event.cpp Map_Manager.cpp Map_T.cpp Message_Block_T.cpp Message_Queue_T.cpp Module.cpp Node.cpp Obstack_T.cpp Pair_T.cpp RB_Tree.cpp Reactor_Token_T.cpp Refcountable_T.cpp Refcounted_Auto_Ptr.cpp Reverse_Lock_T.cpp Select_Reactor_T.cpp Singleton.cpp Strategies_T.cpp Stream.cpp Stream_Modules.cpp String_Base.cpp Svc_Handler.cpp TSS_T.cpp Task_Ex_T.cpp Task_T.cpp Time_Policy_T.cpp Time_Value_T.cpp Timeprobe_T.cpp Timer_Hash_T.cpp Timer_Heap_T.cpp Timer_List_T.cpp Timer_Queue_Adapters.cpp Timer_Queue_Iterator.cpp Timer_Queue_T.cpp Timer_Wheel_T.cpp Tokenizer_T.cpp Typed_SV_Message.cpp Typed_SV_Message_Queue.cpp Unbounded_Queue.cpp Unbounded_Set.cpp Unbounded_Set_Ex.cpp Vector_T.cpp)

set(SOURCE_FILES_ACE_FOR_OPENDDS ACE.cpp ACE_crc32.cpp Addr.cpp Argv_Type_Converter.cpp Assert.cpp Atomic_Op.cpp Atomic_Op_Sparc.c Base_Thread_Adapter.cpp Based_Pointer_Repository.cpp Basic_Stats.cpp Basic_Types.cpp CDR_Base.cpp CDR_Size.cpp CDR_Stream.cpp Cleanup.cpp Condition_Attributes.cpp Condition_Recursive_Thread_Mutex.cpp Condition_Thread_Mutex.cpp Configuration.cpp Configuration_Import_Export.cpp Containers.cpp Copy_Disabled.cpp DLL.cpp DLL_Manager.cpp Dev_Poll_Reactor.cpp Dirent.cpp Dynamic_Message_Strategy.cpp Dynamic_Service_Base.cpp Dynamic_Service_Dependency.cpp Encoding_Converter.cpp Encoding_Converter_Factory.cpp Event_Base.cpp Event_Handler.cpp Event_Handler_Handle_Timeout_Upcall.cpp Flag_Manip.cpp Framework_Component.cpp Functor.cpp Functor_String.cpp Get_Opt.cpp Handle_Set.cpp Hashable.cpp High_Res_Timer.cpp INET_Addr.cpp IO_Cntl_Msg.cpp IPC_SAP.cpp Init_ACE.cpp Lib_Find.cpp Local_Memory_Pool.cpp Lock.cpp Log_Category.cpp Log_Msg.cpp Log_Msg_Backend.cpp Log_Msg_Callback.cpp Log_Msg_IPC.cpp Log_Msg_NT_Event_Log.cpp Log_Msg_UNIX_Syslog.cpp Log_Record.cpp Logging_Strategy.cpp MMAP_Memory_Pool.cpp Malloc.cpp Malloc_Allocator.cpp Mem_Map.cpp Message_Block.cpp Message_Queue.cpp Message_Queue_NT.cpp Message_Queue_Vx.cpp Monotonic_Time_Policy.cpp Mutex.cpp Notification_Queue.cpp Notification_Strategy.cpp Null_Mutex.cpp OS_Errno.cpp OS_Log_Msg_Attributes.cpp OS_NS_Thread.cpp OS_NS_arpa_inet.cpp OS_NS_ctype.cpp OS_NS_dirent.cpp OS_NS_dlfcn.cpp OS_NS_errno.cpp OS_NS_fcntl.cpp OS_NS_math.cpp OS_NS_netdb.cpp OS_NS_poll.cpp OS_NS_pwd.cpp OS_NS_regex.cpp OS_NS_signal.cpp OS_NS_stdio.cpp OS_NS_stdlib.cpp OS_NS_string.cpp OS_NS_strings.cpp OS_NS_stropts.cpp OS_NS_sys_mman.cpp OS_NS_sys_msg.cpp OS_NS_sys_resource.cpp OS_NS_sys_select.cpp OS_NS_sys_sendfile.cpp OS_NS_sys_shm.cpp OS_NS_sys_socket.cpp OS_NS_sys_stat.cpp OS_NS_sys_time.cpp OS_NS_sys_uio.cpp OS_NS_sys_utsname.cpp OS_NS_sys_wait.cpp OS_NS_time.cpp OS_NS_unistd.cpp OS_NS_wchar.cpp OS_NS_wctype.cpp OS_QoS.cpp OS_Thread_Adapter.cpp OS_main.cpp Obchunk.cpp Object_Manager.cpp Object_Manager_Base.cpp Obstack.cpp Parse_Node.cpp Pipe.cpp Process.cpp RW_Mutex.cpp RW_Thread_Mutex.cpp Reactor.cpp Reactor_Impl.cpp Reactor_Timer_Interface.cpp Recursive_Thread_Mutex.cpp Rtems_init.c SOCK.cpp SOCK_Acceptor.cpp SOCK_Connector.cpp SOCK_Dgram.cpp SOCK_Dgram_Mcast.cpp SOCK_IO.cpp SOCK_Stream.cpp SString.cpp Sched_Params.cpp Select_Reactor_Base.cpp Service_Config.cpp Service_Gestalt.cpp Service_Manager.cpp Service_Object.cpp Service_Repository.cpp Service_Types.cpp Shared_Object.cpp Sig_Adapter.cpp Sig_Handler.cpp Signal.cpp Sock_Connect.cpp Stack_Trace.cpp Stats.cpp String_Base_Const.cpp Svc_Conf_Lexer.cpp Svc_Conf_y.cpp TP_Reactor.cpp TSS_Adapter.cpp Task.cpp Thread.cpp Thread_Adapter.cpp Thread_Control.cpp Thread_Exit.cpp Thread_Hook.cpp Thread_Manager.cpp Thread_Mutex.cpp Time_Policy.cpp Time_Value.cpp Token.cpp Trace.cpp UTF16_Encoding_Converter.cpp UTF32_Encoding_Converter.cpp UTF8_Encoding_Converter.cpp XML_Svc_Conf.cpp ace_wchar.cpp)

## Always install these headers regardless of platform
list(APPEND HEADER_FILES_ACE_FOR_OPENDDS Asynch_IO.h Asynch_IO_Impl.h
     Asynch_Pseudo_Task.h Proactor.h Proactor_Impl.h WIN32_Asynch_IO.h
     WIN32_Proactor.h Process_Mutex.h Semaphore.h SV_Semaphore_Complex.h
     SV_Semaphore_Simple.h WFMO_Reactor.h)
list(APPEND INLINE_FILES_ACE_FOR_OPENDDS Asynch_IO_Impl.inl Proactor.inl
     Process_Mutex.inl Semaphore.inl SV_Semaphore_Complex.inl
     SV_Semaphore_Simple.inl WFMO_Reactor.inl)

if(WIN32)
  list(APPEND SOURCE_FILES_ACE_FOR_OPENDDS
       Asynch_IO.cpp Asynch_IO_Impl.cpp Asynch_Pseudo_Task.cpp
       Semaphore.cpp SV_Semaphore_Complex.cpp SV_Semaphore_Simple.cpp
       Proactor.cpp Proactor_Impl.cpp Process_Mutex.cpp
       WIN32_Asynch_IO.cpp WIN32_Proactor.cpp WFMO_Reactor.cpp)
endif()

set(PROJECT_TARGET_ACE_FOR_OPENDDS ACE_for_OpenDDS${LIBRARY_DECORATOR})
if(MSVC)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS
                          _CRT_SECURE_NO_DEPRECATE
                          _CRT_NONSTDC_NO_DEPRECATE)
  add_compile_options(/wd4355)
  add_compile_definitions(_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()

if(ACE_XERCES3)
  find_package(Threads REQUIRED)
  find_package(XercesC PATHS "${ACE_XERCES3}" NO_DEFAULT_PATH)
  if (NOT XercesC_FOUND)
    find_package(XercesC)
  endif()
  if (NOT XercesC_FOUND)
    message(FATAL_ERROR "Could not find XercesC")
  endif()
  set(HEADER_FILES_ACE_XML_UTILS_XMLSCHEMA
      XML_Utils/XMLSchema/Traversal.hpp XML_Utils/XMLSchema/TypeInfo.hpp
      XML_Utils/XMLSchema/Types.hpp XML_Utils/XMLSchema/Writer.hpp
      XML_Utils/XMLSchema/id_map.hpp XML_Utils/XMLSchema/TypeInfo.tpp)
  set(HEADER_FILES_ACE_XML_UTILS
      XML_Utils/XML_Error_Handler.h XML_Utils/XML_Helper.h
      XML_Utils/XML_Schema_Resolver.h XML_Utils/XML_Utils_Export.h
      XML_Utils/XercesString.h XML_Utils/XML_Helper.tpp
      XML_Utils/XML_Schema_Resolver.tpp)
  set(HEADER_FILES_ACE_XML_UTILS_XSCRT
      XML_Utils/XSCRT/Elements.hpp XML_Utils/XSCRT/ExtendedTypeInfo.hpp
      XML_Utils/XSCRT/Parser.hpp XML_Utils/XSCRT/Traversal.hpp
      XML_Utils/XSCRT/Writer.hpp XML_Utils/XSCRT/XML.hpp
      XML_Utils/XSCRT/XMLSchema.hpp XML_Utils/XSCRT/ExtendedTypeInfo.ipp
      XML_Utils/XSCRT/Traversal.tpp)

  list(APPEND SOURCE_FILES_ACE_FOR_OPENDDS
       XML_Utils/XML_Error_Handler.cpp
       XML_Utils/XercesString.cpp
       XML_Utils/XML_Schema_Resolver.cpp)

  install(FILES ${HEADER_FILES_ACE_XML_UTILS}
          DESTINATION "ACE/ace/XML_Utils")
  install(FILES ${HEADER_FILES_ACE_XML_UTILS_XMLSCHEMA}
          DESTINATION "ACE/ace/XML_Utils/XMLSchema")
  install(FILES ${HEADER_FILES_ACE_XML_UTILS_XSCRT}
          DESTINATION "ACE/ace/XML_Utils/XSCRT")
endif()

add_library(${PROJECT_TARGET_ACE_FOR_OPENDDS} ${SOURCE_FILES_ACE_FOR_OPENDDS})
if(BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_TARGET_ACE_FOR_OPENDDS} PRIVATE
                             ACE_BUILD_DLL XML_UTILS_BUILD_DLL)
else()
  target_compile_definitions(${PROJECT_TARGET_ACE_FOR_OPENDDS} PRIVATE
                             ACE_AS_STATIC_LIBS)
endif()
target_link_libraries(${PROJECT_TARGET_ACE_FOR_OPENDDS} ${XercesC_LIBRARIES})
set_target_properties(${PROJECT_TARGET_ACE_FOR_OPENDDS} PROPERTIES
                      DEBUG_POSTFIX "d")

if(ACE_XERCES3)
  target_include_directories(${PROJECT_TARGET_ACE_FOR_OPENDDS} PRIVATE
                             ${XercesC_INCLUDE_DIRS} XML_Utils)
endif()

if(CMAKE_CONFIGURATION_TYPES)
  target_compile_definitions(${PROJECT_TARGET_ACE_FOR_OPENDDS} PUBLIC
                             $<$<CONFIG:Debug>:MPC_LIB_MODIFIER="${LIBRARY_DECORATOR}d">
                             $<$<CONFIG:Release>:MPC_LIB_MODIFIER="${LIBRARY_DECORATOR}">)
endif()

target_include_directories(${PROJECT_TARGET_ACE_FOR_OPENDDS} PRIVATE
                           ${ACE_ROOT})

target_compile_definitions(${PROJECT_TARGET_ACE_FOR_OPENDDS} PUBLIC
                           ACE_LACKS_ACE_TOKEN ACE_LACKS_MSG_WFMO)


install(FILES ${HEADER_FILES_ACE_FOR_OPENDDS}
              ${INLINE_FILES_ACE_FOR_OPENDDS}
              ${TEMPLATE_FILES_ACE_FOR_OPENDDS}
        DESTINATION "ACE/ace")
install(FILES ${HEADER_FILES_ACE_FOR_OPENDDS_OS}
        DESTINATION "ACE/ace/os_include")
install(FILES ${HEADER_FILES_ACE_FOR_OPENDDS_ARPA}
        DESTINATION "ACE/ace/os_include/arpa")
install(FILES ${HEADER_FILES_ACE_FOR_OPENDDS_NET}
        DESTINATION "ACE/ace/os_include/net")
install(FILES ${HEADER_FILES_ACE_FOR_OPENDDS_NETINET}
        DESTINATION "ACE/ace/os_include/netinet")
install(FILES ${HEADER_FILES_ACE_FOR_OPENDDS_SYS}
        DESTINATION "ACE/ace/os_include/sys")

install(TARGETS ${PROJECT_TARGET_ACE_FOR_OPENDDS}
        DESTINATION "ACE/lib")

install(FILES ../cmake/ACE_for_OpenDDSConfig.cmake
        DESTINATION "cmake")
