<script>
	import { page } from '$app/state';
</script>

<main>
	<section class="bg-hero p-16 shadow-md">
		<div class="mx-auto max-w-6xl">
			<div class="flex flex-col items-center justify-center gap-12 text-white">
				<p class="text-4xl">{page.error?.message}</p>
				<a class="link" href="https://mupen64.com">Back to the Homepage</a>
			</div>
		</div>
	</section>
</main>
