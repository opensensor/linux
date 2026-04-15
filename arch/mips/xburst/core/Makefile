obj-y += prom.o

# mxu-v2-ex.obj is a precompiled blob using init_timer_key (removed in 4.15+)
# TODO: replace with source-based MXU2 COP2 exception handler
# obj-y += mxu-v2-ex.obj

obj-$(CONFIG_XBURST_CPU_SCACHE)	+= sc.o
obj-$(CONFIG_SMP)	+= smp.o
obj-$(CONFIG_PMON_DEBUG)       += perf_proc_jz.o perf_event_jz.o

