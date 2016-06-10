namespace CssMinify
{
    using System;
    using System.Text.RegularExpressions;

    class Program
    {
        static void Main(string[] args)
        {
            Test();
            Console.ReadKey();
        }

        private void ProcessArgs(string[] args)
        {
            if (args == null || args.Length == 0)
            {
                Console.WriteLine("123");
                Console.Out.WriteLine("out");
                return;
            }

            if (args.Length == 1 && string.Compare(args[0], "help", StringComparison.OrdinalIgnoreCase) == 0)
            {
                return;
            }
            var inputFile = args[0];
            var outputFile = args[1];
        }

        private static void Test()
        {
            var html =
                "	<a href=\"baidu.com\">\n" +
                "	<link type=\"text/css\" href=\"css/1.css\"	rel=\"stylesheet\"/>\n" +
                "	<link type=\"text/css\" href=\"css/2.css\" rel=\"stylesheet\"/>\n" +
                "	<link type=\"text/css\" href=\"css/3.css\"	rel=\"stylesheet\"/>\n" +
                "	<link type=\"text/css\" href=\"css/4.css\" rel=\"stylesheet\"/>\n" +
                "	<link type=\"text/css\" href=\"css/5.css\" rel=\"stylesheet\"/>\n" +
                "	<link type=\"text/css\" href=\"css/6.css\"	rel=\"stylesheet\"/>\n" +
                "	<link type=\"text/css\" href=\"css/7.css\" rel=\"stylesheet\"/>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/1.js\"></script>\n" +
                "    <link type=\"text/css\" href=\"css/8.css\" rel=\"stylesheet\"/>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/2.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/3.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/4.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/5.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/6.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/7.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/8.js\"></script>\n" +
                "	<script	type=\"text/javascript\" src=\"javascript/9.js\"></script>\n" +
                "</head>\n";
            Console.WriteLine(CustomReplaceCssHead(html));
        }

        private static string CustomReplace(Match m)
        {
            return m.Groups[0].Value;
            // return m.Groups[1].Value;
        }

        private static string CustomReplaceCssHead(string html)
        {
            var evaluator = new MatchEvaluator(CustomReplace);
            var pattern = ".*link.*href=[\" |\\']?(.*[\\\\\\|\\/]?.*)\\.css[\"|\\']?.*";
            var regex = new Regex(pattern, RegexOptions.IgnoreCase | RegexOptions.Multiline);
            return regex.Replace(html, evaluator);
        }

        private static string ReplaceCssHead(string html)
        {
            // var pattern = "<link type=\"text/css\" href=\"";
            // var pattern = "<[^>]+(?:src|href)=\s*["']?([^"]+\.(?:js|css))";

            // .*link.*href=[" |\']?(.*[\\\|\/]?.*)\.css["|\']?.*
            var pattern = ".*link.*href=[\" |\\']?(.*[\\\\\\|\\/]?.*)\\.css[\"|\\']?.*";
            var newCss = "<link type=\"text/css\" href=\"all.min.css\" rel=\"stylesheet\"/>";

            if (!Regex.IsMatch(html, pattern))
            {
                return html;
            }

            html = Regex.Replace(html, pattern, newCss, RegexOptions.Multiline);

            return html;
        }
    }
}
