<template>
  <button
    type="button"
    class="text-red-700 border border-red-700 font-medium rounded-lg text-sm px-5 py-2.5 text-center me-2 mb-2
      hover:text-white hover:bg-red-800
      focus:ring-4 focus:outline-none focus:ring-red-300
      disabled:opacity-50 disabled:pointer-events-none"
  >
    <slot/>
  </button>
</template>
