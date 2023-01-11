ZSTD = True
SZ = True
ZFP = True

#==============================================================================
# Fichiers C++
#==============================================================================
cpp_srcs = ["Compressor/deltaIndex.cpp",
            "Compressor/writeUnsteadyCoefs.cpp",
            "Compressor/cellNCompressor.cpp",
            "Compressor/fpcCompressor.cpp",
            #"Compressor/fpc.cpp",
            "Compressor/indicesCompressor.cpp",
            "Compressor/NGonConnectivityCompressor.cpp"]

if ZSTD:
	zstd_srcs = ["Compressor/zstd/common/debug.c",
	"Compressor/zstd/common/entropy_common.c",
	"Compressor/zstd/common/error_private.c",
	"Compressor/zstd/common/fse_decompress.c",
	"Compressor/zstd/common/pool.c",
	"Compressor/zstd/common/threading.c",
	"Compressor/zstd/common/xxhash.c",
	"Compressor/zstd/common/zstd_common.c",
	"Compressor/zstd/common/error_private.c",
	"Compressor/zstd/common/fse_decompress.c",
	"Compressor/zstd/common/pool.c",
	"Compressor/zstd/common/threading.c",
	"Compressor/zstd/common/xxhash.c",
	"Compressor/zstd/common/zstd_common.c",
	"Compressor/zstd/dictBuilder/cover.c",
	"Compressor/zstd/dictBuilder/divsufsort.c",
	"Compressor/zstd/dictBuilder/zdict.c",
	"Compressor/zstd/decompress/huf_decompress.c",
	"Compressor/zstd/decompress/zstd_decompress.c",
	"Compressor/zstd/compress/fse_compress.c",
	"Compressor/zstd/compress/huf_compress.c",
	"Compressor/zstd/compress/zstd_double_fast.c",
	"Compressor/zstd/compress/zstd_lazy.c",
	"Compressor/zstd/compress/zstd_opt.c",
	"Compressor/zstd/compress/hist.c",
	"Compressor/zstd/compress/zstd_compress.c",
	"Compressor/zstd/compress/zstd_fast.c",
	"Compressor/zstd/compress/zstd_ldm.c",
	"Compressor/zstd/compress/zstdmt_compress.c",
	"Compressor/zstd/legacy/zstd_v01.c",
	"Compressor/zstd/legacy/zstd_v02.c",
	"Compressor/zstd/legacy/zstd_v03.c",
	"Compressor/zstd/legacy/zstd_v04.c",
	"Compressor/zstd/legacy/zstd_v05.c",
	"Compressor/zstd/legacy/zstd_v06.c",
	"Compressor/zstd/legacy/zstd_v07.c"
	]
else: zstd_srcs = []

if SZ:
	sz_srcs = ["Compressor/sz/src/ArithmeticCoding.c",
	"Compressor/sz/src/exafelSZ.c",
	"Compressor/sz/src/sz_float.c",
	"Compressor/sz/src/szd_double.c",
	"Compressor/sz/src/szd_uint64.c",
	"Compressor/sz/src/ByteToolkit.c",
	"Compressor/sz/src/Huffman.c",
	"Compressor/sz/src/sz_float_pwr.c",
	"Compressor/sz/src/szd_double_pwr.c",
	"Compressor/sz/src/szd_uint8.c",
	"Compressor/sz/src/CacheTable.c",
	"Compressor/sz/src/iniparser.c",
	"Compressor/sz/src/sz_float_ts.c",
	"Compressor/sz/src/szd_double_ts.c",
	"Compressor/sz/src/szf.c",
	"Compressor/sz/src/callZlib.c",
	"Compressor/sz/src/MultiLevelCacheTable.c",
	"Compressor/sz/src/sz_int16.c",
	"Compressor/sz/src/szd_float.c",
	"Compressor/sz/src/TightDataPointStorageD.c",
	"Compressor/sz/src/CompressElement.c",
	"Compressor/sz/src/MultiLevelCacheTableWideInterval.c",
	"Compressor/sz/src/sz_int32.c",
	"Compressor/sz/src/szd_float_pwr.c",
	"Compressor/sz/src/TightDataPointStorageF.c",
	"Compressor/sz/src/conf.c",
	"Compressor/sz/src/pastri.c",
	"Compressor/sz/src/sz_int64.c",
	"Compressor/sz/src/szd_float_ts.c",
	"Compressor/sz/src/TightDataPointStorageI.c",
	"Compressor/sz/src/dataCompression.c",
	"Compressor/sz/src/rw.c",
	"Compressor/sz/src/sz_int8.c",
	"Compressor/sz/src/szd_int16.c",
	"Compressor/sz/src/TypeManager.c",
	"Compressor/sz/src/dictionary.c",
	"Compressor/sz/src/rwf.c",
	"Compressor/sz/src/sz_omp.c",
	"Compressor/sz/src/szd_int32.c",
	"Compressor/sz/src/utility.c",
	"Compressor/sz/src/DynamicByteArray.c",
	"Compressor/sz/src/sz.c",
	"Compressor/sz/src/sz_uint16.c",
	"Compressor/sz/src/szd_int64.c",
	"Compressor/sz/src/VarSet.c",
	"Compressor/sz/src/DynamicDoubleArray.c",
	"Compressor/sz/src/sz_double.c",
	"Compressor/sz/src/sz_uint32.c",
	"Compressor/sz/src/szd_int8.c",
	"Compressor/sz/src/DynamicFloatArray.c",
	"Compressor/sz/src/sz_double_pwr.c",
	"Compressor/sz/src/sz_uint64.c",
	"Compressor/sz/src/szd_uint16.c",
	"Compressor/sz/src/DynamicIntArray.c",
	"Compressor/sz/src/sz_double_ts.c",
	"Compressor/sz/src/sz_uint8.c",
	"Compressor/sz/src/szd_uint32.c"
	]
else: sz_srcs = []

if ZFP:
	zfp_srcs = ["Compressor/zfp/src/bitstream.c",
	"Compressor/zfp/src/decode1l.c",
	"Compressor/zfp/src/decode2l.c",
	"Compressor/zfp/src/decode3l.c",
	"Compressor/zfp/src/decode4l.c",
	"Compressor/zfp/src/encode1l.c",
	"Compressor/zfp/src/encode2l.c",
	"Compressor/zfp/src/encode3l.c",
	"Compressor/zfp/src/encode4l.c",
	"Compressor/zfp/src/decode1d.c",
	"Compressor/zfp/src/decode2d.c",
	"Compressor/zfp/src/decode3d.c",
	"Compressor/zfp/src/decode4d.c",
	"Compressor/zfp/src/encode1d.c",
	"Compressor/zfp/src/encode2d.c",
	"Compressor/zfp/src/encode3d.c",
	"Compressor/zfp/src/encode4d.c",
	"Compressor/zfp/src/zfp.c",
	"Compressor/zfp/src/decode1f.c",
	"Compressor/zfp/src/decode2f.c",
	"Compressor/zfp/src/decode3f.c",
	"Compressor/zfp/src/decode4f.c",
	"Compressor/zfp/src/encode1f.c",
	"Compressor/zfp/src/encode2f.c",
	"Compressor/zfp/src/encode3f.c",
	"Compressor/zfp/src/encode4f.c",
	"Compressor/zfp/src/decode1i.c",
	"Compressor/zfp/src/decode2i.c",
	"Compressor/zfp/src/decode3i.c",
	"Compressor/zfp/src/decode4i.c",  
	"Compressor/zfp/src/encode1i.c",
	"Compressor/zfp/src/encode2i.c",
	"Compressor/zfp/src/encode3i.c",
	"Compressor/zfp/src/encode4i.c"
	]
else: zfp_srcs = []
#==============================================================================
# Fichiers fortran
#==============================================================================
fortran_srcs = []
