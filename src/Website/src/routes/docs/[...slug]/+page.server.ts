import { Marked } from 'marked';
import type { PageServerLoad } from './$types';
import { redirect, error } from '@sveltejs/kit';
import { get_doc_by_path, get_first_doc_path } from '$lib/helpers/DocFetcher';

export const load: PageServerLoad = async ({ params }) => {
  const doc = get_doc_by_path(params.slug);

  if (!doc || doc.is_dir) {
    const first = get_first_doc_path();
    if (!first) {
      error(404, 'No documentation found');
    }
    redirect(307, `/docs/${first}`);
  }

  const marked = new Marked({
    hooks: {
      postprocess(html) {
        html = html.replaceAll('[!NOTE]', '<div class="note">');
        html = html.replaceAll('[!NOTE---]', '</div>');
        html = html.replaceAll('[!TIP]', '<div class="tip">');
        html = html.replaceAll('[!TIP---]', '</div>');
        html = html.replaceAll('[!WARN]', '<div class="warn">');
        html = html.replaceAll('[!WARN---]', '</div>');
        html = html.replaceAll('[!CAUTION]', '<div class="caution">');
        html = html.replaceAll('[!CAUTION---]', '</div>');
        html = html.replaceAll('<p', '<p class="y-2"');
        html = html.replaceAll('<a', '<a class="link"');
        html = html.replaceAll('<h1', '<h1 class="my-4 text-3xl font-bold separator-below"');
        html = html.replaceAll('<h2', '<h2 class="my-3 text-2xl"');
        html = html.replaceAll('<h3', '<h3 class="my-2 text-xl"');
        html = html.replaceAll('<code', '<code class="font-mono bg-base-300 px-1"');
        html = html.replaceAll(
          '<table',
          '<table class="my-4 w-full overflow-hidden table table-zebra bg-base-300"'
        );
        html = html.replaceAll(
          '<ol',
          '<ol class="ol"'
        );

        return html
      }
    }
  });

  const html = await marked.parse(doc.content!);

  return {
    content: html,
    title: doc.title,
    path: doc.path,
  };
};
