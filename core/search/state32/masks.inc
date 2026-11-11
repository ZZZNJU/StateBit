// ==== shared avxPopcnt8x32 ====
        inline __m256i avxPopcnt8x32(__m256i x) const {
            const __m256i lut=_mm256_setr_epi8(
                0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,
                0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
            const __m256i lowMask=_mm256_set1_epi8(0x0f);
            const __m256i lo=_mm256_and_si256(x,lowMask);
            const __m256i hi=_mm256_and_si256(_mm256_srli_epi16(x,4),lowMask);
            const __m256i bytes=_mm256_add_epi8(_mm256_shuffle_epi8(lut,lo),_mm256_shuffle_epi8(lut,hi));
            const __m256i sum16=_mm256_maddubs_epi16(bytes,_mm256_set1_epi8(1));
            return _mm256_madd_epi16(sum16,_mm256_set1_epi16(1));
        }

// ==== shared avxActiveMask ====
        inline uint8_t avxActiveMask(const int active) const {
            return active>=8?0xffu:static_cast<uint8_t>((1u<<active)-1u);
        }

// ==== shared avxLaneMaskVec ====
        inline __m256i avxLaneMaskVec(const uint8_t laneMask) const {
            alignas(32) int v[8];
            for(int i=0;i<8;i++) v[i]=(laneMask&(1u<<i))?-1:0;
            return _mm256_load_si256(reinterpret_cast<const __m256i*>(v));
        }

// ==== shared avxNonZeroLaneMask ====
        inline uint8_t avxNonZeroLaneMask(const __m256i v) const {
            const __m256i z=_mm256_setzero_si256();
            const uint8_t isZero=static_cast<uint8_t>(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(v,z))));
            return static_cast<uint8_t>(~isZero);
        }

// ==== shared avxCompactVector ====
        inline __m256i avxCompactVector(const __m256i v,const uint8_t mask) const {
            static const AvxPermTable table;
            const __m256i perm=_mm256_load_si256(reinterpret_cast<const __m256i*>(table.idx[mask]));
            return _mm256_permutevar8x32_epi32(v,perm);
        }

// ==== shared avxPushCompacted ====
        inline void avxPushCompacted(vector<LaneState32>& stack,const uint32_t P[8],const uint32_t C[8],const uint32_t X[8],const uint8_t mask){
            if(!mask) return;
            AVX_COUNT(++avxStats.compaction_calls);
            const int n=__builtin_popcount(static_cast<unsigned>(mask));
            __m256i pv=avxCompactVector(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(P)),mask);
            __m256i cv=avxCompactVector(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(C)),mask);
            __m256i xv=avxCompactVector(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(X)),mask);
            alignas(32) uint32_t pp[8],cc[8],xx[8];
            _mm256_store_si256(reinterpret_cast<__m256i*>(pp),pv);
            _mm256_store_si256(reinterpret_cast<__m256i*>(cc),cv);
            _mm256_store_si256(reinterpret_cast<__m256i*>(xx),xv);
            for(int i=0;i<n;i++) stack.push_back({pp[i],cc[i],xx[i],false});
            AVX_COUNT(avxStats.compacted_children += n);
        }

// ==== shared laneCompress ====
        inline uint32_t laneCompress(const uint64_t mask,const LaneContext32& ctx) const {
#if LANE_USE_BMI2
            LANE_COUNT(++laneStats.pext_calls);
            return static_cast<uint32_t>(_pext_u64(mask,ctx.rootMask));
#else
            uint32_t out=0;
            for(int i=0;i<ctx.m;i++) if(mask&(1ULL<<ctx.orig[i])) out|=1U<<i;
            return out;
#endif
        }

// ==== shared laneExpand ====
        inline uint64_t laneExpand(const uint32_t mask,const LaneContext32& ctx) const {
#if LANE_USE_BMI2
            LANE_COUNT(++laneStats.pdep_calls);
            return _pdep_u64(static_cast<uint64_t>(mask),ctx.rootMask);
#else
            uint64_t out=0;
            uint32_t t=mask;
            while(t){ const int i=laneFirst(t); t&=t-1; out|=1ULL<<ctx.orig[i]; }
            return out;
#endif
        }

// ==== shared lanePop ====
        inline int lanePop(const uint32_t x) const { return __builtin_popcount(x); }

// ==== shared laneFirst ====
        inline int laneFirst(const uint32_t x) const { return __builtin_ctz(x); }

// ==== shared laneLo ====
        inline uint32_t laneLo(const uint64_t x) const { return static_cast<uint32_t>(x); }

// ==== shared laneHi ====
        inline uint32_t laneHi(const uint64_t x) const { return static_cast<uint32_t>(x>>32); }

// ==== shared lanePack ====
        inline uint64_t lanePack(const uint32_t lo,const uint32_t hi) const {
            return static_cast<uint64_t>(lo)|(static_cast<uint64_t>(hi)<<32);
        }

// ==== shared laneRowsPack ====
        inline uint64_t laneRowsPack(const uint32_t rows[32],const int i0,const int i1) const {
            return lanePack(i0>=0?rows[i0]:0U,i1>=0?rows[i1]:0U);
        }

// ==== shared laneSaturated ====
        inline uint32_t laneSaturated(const uint32_t P,const LaneContext32& ctx) const {
            uint32_t sat=0,t=P;
            while(t){
                const int u=laneFirst(t); t&=t-1;
                if(lanePop(ctx.nonadj[u]&P)==k-1) sat|=1U<<u;
            }
            return sat;
        }
