// RUN: %clang_cc1 -verify -fopenmp -x c++ -triple x86_64-apple-darwin10 -emit-llvm %s -o - | FileCheck %s
// RUN: %clang_cc1 -fopenmp -x c++ -std=c++11 -triple x86_64-apple-darwin10 -emit-pch -o %t %s
// RUN: %clang_cc1 -fopenmp -x c++ -triple x86_64-apple-darwin10 -std=c++11 -include-pch %t -verify %s -emit-llvm -o - | FileCheck %s
// RUN: %clang_cc1 -verify -fopenmp -x c++ -std=c++11 -DLAMBDA -triple x86_64-apple-darwin10 -emit-llvm %s -o - | FileCheck -check-prefix=LAMBDA %s
// RUN: %clang_cc1 -verify -fopenmp -x c++ -fblocks -DBLOCKS -triple x86_64-apple-darwin10 -emit-llvm %s -o - | FileCheck -check-prefix=BLOCKS %s
// expected-no-diagnostics
#ifndef HEADER
#define HEADER

struct St {
  int a, b;
  St() : a(0), b(0) {}
  St(const St &st) : a(st.a + st.b), b(0) {}
  ~St() {}
};

volatile int g = 1212;

template <class T>
struct S {
  T f;
  S(T a) : f(a + g) {}
  S() : f(g) {}
  S(const S &s, St t = St()) : f(s.f + t.a) {}
  operator T() { return T(); }
  ~S() {}
};

// CHECK-DAG: [[S_FLOAT_TY:%.+]] = type { float }
// CHECK-DAG: [[S_INT_TY:%.+]] = type { i{{[0-9]+}} }
// CHECK-DAG: [[ST_TY:%.+]] = type { i{{[0-9]+}}, i{{[0-9]+}} }

template <typename T>
T tmain() {
  S<T> test;
  T t_var = T();
  T vec[] = {1, 2};
  S<T> s_arr[] = {1, 2};
  S<T> var(3);
#pragma omp parallel
#pragma omp sections firstprivate(t_var, vec, s_arr, var)
  {
    vec[0] = t_var;
#pragma omp section
    s_arr[0] = var;
  }
  return T();
}

// CHECK: [[TEST:@.+]] = global [[S_FLOAT_TY]] zeroinitializer,
S<float> test;
// CHECK-DAG: [[T_VAR:@.+]] = global i{{[0-9]+}} 333,
int t_var = 333;
// CHECK-DAG: [[VEC:@.+]] = global [2 x i{{[0-9]+}}] [i{{[0-9]+}} 1, i{{[0-9]+}} 2],
int vec[] = {1, 2};
// CHECK-DAG: [[S_ARR:@.+]] = global [2 x [[S_FLOAT_TY]]] zeroinitializer,
S<float> s_arr[] = {1, 2};
// CHECK-DAG: [[VAR:@.+]] = global [[S_FLOAT_TY]] zeroinitializer,
S<float> var(3);
// CHECK-DAG: [[SIVAR:@.+]] = internal global i{{[0-9]+}} 0,
// CHECK-DAG: [[SECTIONS_BARRIER_LOC:@.+]] = private unnamed_addr constant %{{.+}} { i32 0, i32 194, i32 0, i32 0, i8*

// CHECK: call {{.*}} [[S_FLOAT_TY_DEF_CONSTR:@.+]]([[S_FLOAT_TY]]* [[TEST]])
// CHECK: ([[S_FLOAT_TY]]*)* [[S_FLOAT_TY_DESTR:@[^ ]+]] {{[^,]+}}, {{.+}}([[S_FLOAT_TY]]* [[TEST]]
int main() {
  static int sivar;
#ifdef LAMBDA
  // LAMBDA: [[G:@.+]] = global i{{[0-9]+}} 1212,
  // LAMBDA-LABEL: @main
  // LAMBDA: call void [[OUTER_LAMBDA:@.+]](
  [&]() {
// LAMBDA: define{{.*}} internal{{.*}} void [[OUTER_LAMBDA]](
// LAMBDA: call void {{.+}} @__kmpc_fork_call({{.+}}, i32 1, {{.+}}* [[OMP_REGION:@.+]] to {{.+}})
#pragma omp parallel
#pragma omp sections firstprivate(g, sivar)
  {
    // LAMBDA: define{{.*}} internal{{.*}} void [[OMP_REGION]](i32* noalias %{{.+}}, i32* noalias %{{.+}}, i32* dereferenceable(4) [[SIVAR_REF:%.+]])
    // Skip temp vars for loop
    // LAMBDA: alloca i{{[0-9]+}},
    // LAMBDA: alloca i{{[0-9]+}},
    // LAMBDA: alloca i{{[0-9]+}},
    // LAMBDA: alloca i{{[0-9]+}},
    // LAMBDA: alloca i{{[0-9]+}},
    // LAMBDA: alloca i{{[0-9]+}},

    // LAMBDA: [[G_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}},
    // LAMBDA: [[SIVAR1_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}},
    // LAMBDA: store i{{[0-9]+}}* [[SIVAR_REF]], i{{[0-9]+}}** [[SIVAR_REF_ADDR:%.+]],
    // LAMBDA: [[SIVAR1_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[SIVAR_REF_ADDR]],
    // LAMBDA: [[G_VAL:%.+]] = load volatile i{{[0-9]+}}, i{{[0-9]+}}* [[G]]
    // LAMBDA: store i{{[0-9]+}} [[G_VAL]], i{{[0-9]+}}* [[G_PRIVATE_ADDR]]

    // LAMBDA: [[SIVAR1_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[SIVAR1_REF]]
    // LAMBDA: store i{{[0-9]+}} [[SIVAR1_VAL]], i{{[0-9]+}}* [[SIVAR1_PRIVATE_ADDR]]

    // LAMBDA-NOT: call void @__kmpc_barrier(
    {
      g = 1;
      sivar = 10;
    }
    // LAMBDA: call void @__kmpc_for_static_init_4(
    // LAMBDA: store i{{[0-9]+}} 1, i{{[0-9]+}}* [[G_PRIVATE_ADDR]],
    // LAMBDA: store i{{[0-9]+}} 10, i{{[0-9]+}}* [[SIVAR1_PRIVATE_ADDR]],
    // LAMBDA: [[G_PRIVATE_ADDR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG:%.+]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
    // LAMBDA: store i{{[0-9]+}}* [[G_PRIVATE_ADDR]], i{{[0-9]+}}** [[G_PRIVATE_ADDR_REF]]
    // LAMBDA: [[SIVAR_PRIVATE_ADDR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG:%.+]], i{{[0-9]+}} 0, i{{[0-9]+}} 1
    // LAMBDA: store i{{[0-9]+}}* [[SIVAR1_PRIVATE_ADDR]], i{{[0-9]+}}** [[SIVAR_PRIVATE_ADDR_REF]]
    // LAMBDA: call void [[INNER_LAMBDA:@.+]](%{{.+}}* [[ARG]])
    // LAMBDA: call void @__kmpc_for_static_fini(
    // LAMBDA: call void @__kmpc_barrier(
#pragma omp section
    [&]() {
      // LAMBDA: define {{.+}} void [[INNER_LAMBDA]](%{{.+}}* [[ARG_PTR:%.+]])
      // LAMBDA: store %{{.+}}* [[ARG_PTR]], %{{.+}}** [[ARG_PTR_REF:%.+]],
      g = 2;
      sivar = 20;
      // LAMBDA: [[ARG_PTR:%.+]] = load %{{.+}}*, %{{.+}}** [[ARG_PTR_REF]]
      // LAMBDA: [[G_PTR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG_PTR]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
      // LAMBDA: [[G_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[G_PTR_REF]]
      // LAMBDA: store i{{[0-9]+}} 2, i{{[0-9]+}}* [[G_REF]]
      // LAMBDA: [[SIVAR_PTR_REF:%.+]] = getelementptr inbounds %{{.+}}, %{{.+}}* [[ARG_PTR]], i{{[0-9]+}} 0, i{{[0-9]+}} 1
      // LAMBDA: [[SIVAR_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[SIVAR_PTR_REF]]
      // LAMBDA: store i{{[0-9]+}} 20, i{{[0-9]+}}* [[SIVAR_REF]]
    }();
  }
  }();
  return 0;
#elif defined(BLOCKS)
  // BLOCKS: [[G:@.+]] = global i{{[0-9]+}} 1212,
  // BLOCKS-LABEL: @main
  // BLOCKS: call void {{%.+}}(i8
  ^{
// BLOCKS: define{{.*}} internal{{.*}} void {{.+}}(i8*
// BLOCKS: call void {{.+}} @__kmpc_fork_call({{.+}}, i32 1, {{.+}}* [[OMP_REGION:@.+]] to {{.+}})
#pragma omp parallel
#pragma omp sections firstprivate(g, sivar)
   {
    // BLOCKS: define{{.*}} internal{{.*}} void [[OMP_REGION]](i32* noalias %{{.+}}, i32* noalias %{{.+}}, i32* dereferenceable(4) [[SIVAR_REF:%.+]])
    // Skip temp vars for loop
    // BLOCKS: alloca i{{[0-9]+}},
    // BLOCKS: alloca i{{[0-9]+}},
    // BLOCKS: alloca i{{[0-9]+}},
    // BLOCKS: alloca i{{[0-9]+}},
    // BLOCKS: alloca i{{[0-9]+}},
    // BLOCKS: alloca i{{[0-9]+}},
    // BLOCKS: [[G_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}},
    // BLOCKS: [[SIVAR1_PRIVATE_ADDR:%.+]] = alloca i{{[0-9]+}},

    // BLOCKS: store i{{[0-9]+}}* [[SIVAR_REF]], i{{[0-9]+}}** {{.+}},
    // BLOCKS: [[SIVAR1_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** {{.+}},

    // BLOCKS: [[G_VAL:%.+]] = load volatile i{{[0-9]+}}, i{{[0-9]+}}* [[G]]
    // BLOCKS: store i{{[0-9]+}} [[G_VAL]], i{{[0-9]+}}* [[G_PRIVATE_ADDR]]

    // BLOCKS: [[SIVAR1_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[SIVAR1_REF]],
    // BLOCKS: store i{{[0-9]+}} [[SIVAR1_VAL]], i{{[0-9]+}}* [[SIVAR1_PRIVATE_ADDR]],
    // BLOCKS-NOT: call void @__kmpc_barrier(
    {
      g = 1;
      sivar = 10;
    }
    // BLOCKS: call void @__kmpc_for_static_init_4(
    // BLOCKS: store i{{[0-9]+}} 1, i{{[0-9]+}}* [[G_PRIVATE_ADDR]],
    // BLOCKS: store i{{[0-9]+}} 10, i{{[0-9]+}}* [[SIVAR1_PRIVATE_ADDR]],
    // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
    // BLOCKS: i{{[0-9]+}}* [[G_PRIVATE_ADDR]]
    // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
    // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
    // BLOCKS: i{{[0-9]+}}* [[SIVAR1_PRIVATE_ADDR]]
    // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
    // BLOCKS: call void {{%.+}}(i8
    // BLOCKS: call void @__kmpc_for_static_fini(
    // BLOCKS: call void @__kmpc_barrier(
#pragma omp section
    ^{
      // BLOCKS: define {{.+}} void {{@.+}}(i8*
      g = 2;
      sivar = 20;
      // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
      // BLOCKS: store i{{[0-9]+}} 2, i{{[0-9]+}}*
      // BLOCKS-NOT: [[G]]{{[[^:word:]]}}
      // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
      // BLOCKS: store i{{[0-9]+}} 20, i{{[0-9]+}}*
      // BLOCKS-NOT: [[SIVAR]]{{[[^:word:]]}}
      // BLOCKS: ret
    }();
  }
  }();
  return 0;
#else
#pragma omp sections firstprivate(t_var, vec, s_arr, var, sivar) nowait
  {
    {
    vec[0] = t_var;
    s_arr[0] = var;
    sivar = 31;
    }
  }
  return tmain<int>();
#endif
}

// CHECK: define {{.*}}i{{[0-9]+}} @main()
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: [[T_VAR_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[VEC_PRIV:%.+]] = alloca [2 x i{{[0-9]+}}],
// CHECK: [[S_ARR_PRIV:%.+]] = alloca [2 x [[S_FLOAT_TY]]],
// CHECK: [[VAR_PRIV:%.+]] = alloca [[S_FLOAT_TY]],
// CHECK: [[SIVAR_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[GTID:%.+]] = call i32 @__kmpc_global_thread_num(

// firstprivate t_var(t_var)
// CHECK: [[T_VAR_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[T_VAR]],
// CHECK: store i{{[0-9]+}} [[T_VAR_VAL]], i{{[0-9]+}}* [[T_VAR_PRIV]],

// firstprivate vec(vec)
// CHECK: [[VEC_DEST:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_PRIV]] to i8*
// CHECK: call void @llvm.memcpy.{{.+}}(i8* [[VEC_DEST]], i8* bitcast ([2 x i{{[0-9]+}}]* [[VEC]] to i8*),

// firstprivate s_arr(s_arr)
// CHECK: [[S_ARR_PRIV_BEGIN:%.+]] = getelementptr inbounds [2 x [[S_FLOAT_TY]]], [2 x [[S_FLOAT_TY]]]* [[S_ARR_PRIV]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
// CHECK: [[S_ARR_PRIV_END:%.+]] = getelementptr [[S_FLOAT_TY]], [[S_FLOAT_TY]]* [[S_ARR_PRIV_BEGIN]], i{{[0-9]+}} 2
// CHECK: [[IS_EMPTY:%.+]] = icmp eq [[S_FLOAT_TY]]* [[S_ARR_PRIV_BEGIN]], [[S_ARR_PRIV_END]]
// CHECK: br i1 [[IS_EMPTY]], label %[[S_ARR_BODY_DONE:.+]], label %[[S_ARR_BODY:.+]]
// CHECK: [[S_ARR_BODY]]
// CHECK: getelementptr inbounds ([2 x [[S_FLOAT_TY]]], [2 x [[S_FLOAT_TY]]]* [[S_ARR]], i{{[0-9]+}} 0, i{{[0-9]+}} 0)
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR:@.+]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_FLOAT_TY_COPY_CONSTR:@.+]]([[S_FLOAT_TY]]* {{.+}}, [[S_FLOAT_TY]]* {{.+}}, [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR:@.+]]([[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: br i1 {{.+}}, label %{{.+}}, label %[[S_ARR_BODY]]

// firstprivate var(var)
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_FLOAT_TY_COPY_CONSTR]]([[S_FLOAT_TY]]* [[VAR_PRIV]], [[S_FLOAT_TY]]* {{.*}} [[VAR]], [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR]]([[ST_TY]]* [[ST_TY_TEMP]])

// firstprivate isvar
// CHECK: [[SIVAR_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[SIVAR]],
// CHECK: store i{{[0-9]+}} [[SIVAR_VAL]], i{{[0-9]+}}* [[SIVAR_PRIV]],

// CHECK-NOT: call void @__kmpc_barrier(
// CHECK: call void @__kmpc_for_static_init_4(
// CHECK: call void @__kmpc_for_static_fini(

// ~(firstprivate var), ~(firstprivate s_arr)
// CHECK-DAG: call {{.*}} [[S_FLOAT_TY_DESTR]]([[S_FLOAT_TY]]* [[VAR_PRIV]])
// CHECK-DAG: call {{.*}} [[S_FLOAT_TY_DESTR]]([[S_FLOAT_TY]]*

// CHECK: = call {{.*}}i{{.+}} [[TMAIN_INT:@.+]]()

// CHECK: ret void

// CHECK: define {{.*}} i{{[0-9]+}} [[TMAIN_INT]]()
// CHECK: [[TEST:%.+]] = alloca [[S_INT_TY]],
// CHECK: call {{.*}} [[S_INT_TY_DEF_CONSTR:@.+]]([[S_INT_TY]]* [[TEST]])
// CHECK: [[T_VARVAL:%.+]] = load i32, i32* %{{.+}},
// CHECK: [[T_VARCONV:%.+]] = bitcast i64* [[T_VARCAST:%.+]] to i32*
// CHECK: store i32  [[T_VARVAL]], i32* [[T_VARCONV]],
// CHECK: [[T_VARPVT:%.+]] = load i64, i64* [[T_VARCAST]],
// CHECK: call void (%{{.+}}*, i{{[0-9]+}}, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)*, ...) @__kmpc_fork_call(%{{.+}}* @{{.+}}, i{{[0-9]+}} 4, void (i{{[0-9]+}}*, i{{[0-9]+}}*, ...)* bitcast (void (i{{[0-9]+}}*, i{{[0-9]+}}*, i64, [2 x i32]*, [2 x [[S_INT_TY]]]*, [[S_INT_TY]]*)* [[TMAIN_MICROTASK:@.+]] to void {{.*}}i64 [[T_VARPVT]],
// CHECK: call {{.*}} [[S_INT_TY_DESTR:@.+]]([[S_INT_TY]]*
// CHECK: ret
//
// CHECK: define internal void [[TMAIN_MICROTASK]](i{{[0-9]+}}* noalias [[GTID_ADDR:%.+]], i{{[0-9]+}}* noalias %{{.+}},
// Skip temp vars for loop
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: alloca i{{[0-9]+}},
// CHECK: [[T_VAR_PRIV:%.+]] = alloca i{{[0-9]+}},
// CHECK: [[VEC_PRIV:%.+]] = alloca [2 x i{{[0-9]+}}],
// CHECK: [[S_ARR_PRIV:%.+]] = alloca [2 x [[S_INT_TY]]],
// CHECK: [[VAR_PRIV:%.+]] = alloca [[S_INT_TY]],
// CHECK: store i{{[0-9]+}}* [[GTID_ADDR]], i{{[0-9]+}}** [[GTID_ADDR_ADDR:%.+]],

// CHECK-NOT: load i{{[0-9]+}}*, i{{[0-9]+}}** %
// CHECK: [[VEC_REF:%.+]] = load [2 x i{{[0-9]+}}]*, [2 x i{{[0-9]+}}]** %
// CHECK: [[S_ARR:%.+]] = load [2 x [[S_INT_TY]]]*, [2 x [[S_INT_TY]]]** %
// CHECK: [[VAR_REF:%.+]] = load [[S_INT_TY]]*, [[S_INT_TY]]** %

// firstprivate t_var(t_var)
// CHECK: [[T_VAR_VAL:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}*
// CHECK: store i{{[0-9]+}} [[T_VAR_VAL]], i{{[0-9]+}}* [[T_VAR_PRIV]],

// firstprivate vec(vec)
// CHECK: [[VEC_DEST:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_PRIV]] to i8*
// CHECK: [[VEC_SRC:%.+]] = bitcast [2 x i{{[0-9]+}}]* [[VEC_REF]] to i8*
// CHECK: call void @llvm.memcpy.{{.+}}(i8* [[VEC_DEST]], i8* [[VEC_SRC]],

// firstprivate s_arr(s_arr)
// CHECK: [[S_ARR_PRIV_BEGIN:%.+]] = getelementptr inbounds [2 x [[S_INT_TY]]], [2 x [[S_INT_TY]]]* [[S_ARR_PRIV]], i{{[0-9]+}} 0, i{{[0-9]+}} 0
// CHECK: [[S_ARR_PRIV_END:%.+]] = getelementptr [[S_INT_TY]], [[S_INT_TY]]* [[S_ARR_PRIV_BEGIN]], i{{[0-9]+}} 2
// CHECK: [[IS_EMPTY:%.+]] = icmp eq [[S_INT_TY]]* [[S_ARR_PRIV_BEGIN]], [[S_ARR_PRIV_END]]
// CHECK: br i1 [[IS_EMPTY]], label %[[S_ARR_BODY_DONE:.+]], label %[[S_ARR_BODY:.+]]
// CHECK: [[S_ARR_BODY]]
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR:@.+]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_INT_TY_COPY_CONSTR:@.+]]([[S_INT_TY]]* {{.+}}, [[S_INT_TY]]* {{.+}}, [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR:@.+]]([[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: br i1 {{.+}}, label %{{.+}}, label %[[S_ARR_BODY]]

// firstprivate var(var)
// CHECK: call {{.*}} [[ST_TY_DEFAULT_CONSTR]]([[ST_TY]]* [[ST_TY_TEMP:%.+]])
// CHECK: call {{.*}} [[S_INT_TY_COPY_CONSTR]]([[S_INT_TY]]* [[VAR_PRIV]], [[S_INT_TY]]* {{.*}} [[VAR_REF]], [[ST_TY]]* [[ST_TY_TEMP]])
// CHECK: call {{.*}} [[ST_TY_DESTR]]([[ST_TY]]* [[ST_TY_TEMP]])

// No synchronization for initialization.
// CHECK-NOT: call void @__kmpc_barrier(

// CHECK: call void @__kmpc_for_static_init_4(
// CHECK: call void @__kmpc_for_static_fini(

// ~(firstprivate var), ~(firstprivate s_arr)
// CHECK-DAG: call {{.*}} [[S_INT_TY_DESTR]]([[S_INT_TY]]* [[VAR_PRIV]])
// CHECK-DAG: call {{.*}} [[S_INT_TY_DESTR]]([[S_INT_TY]]*
// CHECK: [[GTID_REF:%.+]] = load i{{[0-9]+}}*, i{{[0-9]+}}** [[GTID_ADDR_ADDR]]
// CHECK: [[GTID:%.+]] = load i{{[0-9]+}}, i{{[0-9]+}}* [[GTID_REF]]
// CHECK: call void @__kmpc_barrier(%{{.+}}* [[SECTIONS_BARRIER_LOC]], i{{[0-9]+}} [[GTID]])
// CHECK: ret void
#endif

